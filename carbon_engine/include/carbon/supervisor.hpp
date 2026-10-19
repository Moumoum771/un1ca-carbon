#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include "carbon/common.hpp"

namespace carbon::supervisor {

struct CommandSpec {
    std::string executable;
    std::vector<std::string> args;
    fs::path work_dir;
    std::unordered_map<std::string, std::string> env;
    std::chrono::milliseconds timeout{std::chrono::hours(1)};
    bool stream_to_stdout{true};
};

struct ExecutionOutput {
    int exit_code{-1};
    std::string std_out;
    std::string std_err;
    std::chrono::milliseconds elapsed{0};
};

class ProcessSupervisor {
public:
    ProcessSupervisor();
    ~ProcessSupervisor();

    // Executa comando com isolamento de processo e monitoramento de tempo
    CarbonResult<ExecutionOutput> ExecuteSynchronous(const CommandSpec& spec);

    // Transacao atomica de promocao de arquivo (.tmp -> destino final)
    static CarbonResult<void> AtomicRename(const fs::path& source_tmp, const fs::path& final_destination);
};

} // namespace carbon::supervisor
