#include "carbon/supervisor.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <vector>

namespace carbon::supervisor {

ProcessSupervisor::ProcessSupervisor() = default;
ProcessSupervisor::~ProcessSupervisor() = default;

CarbonResult<ExecutionOutput> ProcessSupervisor::ExecuteSynchronous(const CommandSpec& spec) {
    int out_pipe[2];
    int err_pipe[2];

    if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
        return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Failed to create IPC communication pipes");
    }

    auto start_time = std::chrono::steady_clock::now();

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Failed to fork subprocess");
    }

    if (pid == 0) {
        // Processo Filho
        setpgid(0, 0); // Isola o grupo de processos

        close(out_pipe[0]);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[1]);

        close(err_pipe[0]);
        dup2(err_pipe[1], STDERR_FILENO);
        close(err_pipe[1]);

        if (!spec.work_dir.empty()) {
            chdir(spec.work_dir.c_str());
        }

        // Configura variáveis de ambiente extras
        for (const auto& [k, v] : spec.env) {
            setenv(k.c_str(), v.c_str(), 1);
        }

        // Converte argumentos para char* argv[]
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(spec.executable.c_str()));
        for (const auto& arg : spec.args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(spec.executable.c_str(), argv.data());
        // Se execvp falhar
        _exit(127);
    }

    // Processo Pai
    close(out_pipe[1]);
    close(err_pipe[1]);

    // Torna leitura nao-bloqueante
    fcntl(out_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(err_pipe[0], F_SETFL, O_NONBLOCK);

    std::string out_buf;
    std::string err_buf;

    struct pollfd fds[2];
    fds[0].fd = out_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = err_pipe[0];
    fds[1].events = POLLIN;

    char buffer[4096];
    bool out_open = true;
    bool err_open = true;

    while (out_open || err_open) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time) > spec.timeout) {
            // Timeout excedido! Mata todo o grupo de processos
            kill(-pid, SIGTERM);
            usleep(200000); // 200ms grace period
            kill(-pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            close(out_pipe[0]);
            close(err_pipe[0]);
            return CarbonError(ErrorCode::ERR_SUBPROCESS_TIMEOUT, "Processo excedeu tempo limite configurado");
        }

        int poll_res = poll(fds, 2, 100);
        if (poll_res < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (out_open && (fds[0].revents & POLLIN)) {
            ssize_t bytes = read(out_pipe[0], buffer, sizeof(buffer));
            if (bytes > 0) {
                out_buf.append(buffer, static_cast<size_t>(bytes));
                if (spec.stream_to_stdout) {
                    std::cout.write(buffer, bytes);
                    std::cout.flush();
                }
            } else if (bytes == 0) {
                out_open = false;
            }
        } else if (fds[0].revents & (POLLHUP | POLLERR)) {
            out_open = false;
        }

        if (err_open && (fds[1].revents & POLLIN)) {
            ssize_t bytes = read(err_pipe[0], buffer, sizeof(buffer));
            if (bytes > 0) {
                err_buf.append(buffer, static_cast<size_t>(bytes));
                if (spec.stream_to_stdout) {
                    std::cerr.write(buffer, bytes);
                    std::cerr.flush();
                }
            } else if (bytes == 0) {
                err_open = false;
            }
        } else if (fds[1].revents & (POLLHUP | POLLERR)) {
            err_open = false;
        }
    }

    close(out_pipe[0]);
    close(err_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    auto end_time = std::chrono::steady_clock::now();

    ExecutionOutput output;
    output.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    output.std_out = std::move(out_buf);
    output.std_err = std::move(err_buf);
    output.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    return output;
}

CarbonResult<void> ProcessSupervisor::AtomicRename(const fs::path& source_tmp, const fs::path& final_destination) {
    if (!fs::exists(source_tmp)) {
        return CarbonResult<void>::err(ErrorCode::ERR_ATOMIC_RENAME_FAIL, "Arquivo de staging temporario nao existe: " + source_tmp.string());
    }

    std::error_code ec;
    fs::create_directories(final_destination.parent_path(), ec);

    if (rename(source_tmp.c_str(), final_destination.c_str()) != 0) {
        return CarbonResult<void>::err(ErrorCode::ERR_ATOMIC_RENAME_FAIL, "Falha no rename atomico: " + std::string(strerror(errno)));
    }

    return CarbonResult<void>::ok();
}

} // namespace carbon::supervisor
