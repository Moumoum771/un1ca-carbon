#include "carbon/fingerprint.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

namespace carbon::fingerprint {

static bool is_excluded(const std::string& rel_path, const std::vector<std::string>& exclusions) {
    static const std::vector<std::string> default_exclusions = {
        ".git", ".bak", ".orig", ".rej", ".tmp", ".swp", ".DS_Store", "build", "dist"
    };

    for (const auto& exc : default_exclusions) {
        if (rel_path.find(exc) != std::string::npos) return true;
    }
    for (const auto& exc : exclusions) {
        if (rel_path.find(exc) != std::string::npos) return true;
    }
    return false;
}

InputHasher::InputHasher(fs::path root_dir) : root_dir_(std::move(root_dir)) {}
InputHasher::~InputHasher() = default;

CarbonResult<std::string> InputHasher::ComputeFileSha256(const fs::path& file_path) {
    if (!fs::exists(file_path)) {
        return CarbonError(ErrorCode::ERR_HASH_IO_ERROR, "File does not exist: " + file_path.string());
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return CarbonError(ErrorCode::ERR_HASH_IO_ERROR, "Cannot open file for reading: " + file_path.string());
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Failed to allocate OpenSSL EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Failed on EVP_DigestInit_ex");
    }

    constexpr size_t buffer_size = 65536; // 64 KB
    std::vector<char> buffer(buffer_size);
    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buffer.data(), static_cast<size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Falha no EVP_DigestUpdate");
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Falha no EVP_DigestFinal_ex");
    }

    EVP_MD_CTX_free(ctx);
    return to_hex(digest, digest_len);
}

CarbonResult<std::string> InputHasher::ComputeStatHash(const std::vector<std::string>& exclusions) {
    if (!fs::exists(root_dir_)) {
        return CarbonError(ErrorCode::ERR_STAT_SCAN_FAILED, "Root directory does not exist: " + root_dir_.string());
    }

    std::vector<std::string> rel_paths;
    std::error_code ec;

    for (const auto& entry : fs::recursive_directory_iterator(root_dir_, fs::directory_options::skip_permission_denied, ec)) {
        std::string rel = fs::relative(entry.path(), root_dir_, ec).string();
        if (!is_excluded(rel, exclusions)) {
            rel_paths.push_back(rel);
        }
    }

    std::sort(rel_paths.begin(), rel_paths.end());

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    for (const auto& rel : rel_paths) {
        fs::path full_p = root_dir_ / rel;
        struct stat st{};
        if (lstat(full_p.c_str(), &st) == 0) {
            EVP_DigestUpdate(ctx, rel.data(), rel.size());
            EVP_DigestUpdate(ctx, &st.st_size, sizeof(st.st_size));
            EVP_DigestUpdate(ctx, &st.st_mtime, sizeof(st.st_mtime));
            uint32_t mode = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
            EVP_DigestUpdate(ctx, &mode, sizeof(mode));
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    return to_hex(digest, digest_len);
}

CarbonResult<std::string> InputHasher::ComputeDeepContentHash(const std::vector<std::string>& exclusions) {
    if (!fs::exists(root_dir_)) {
        return CarbonError(ErrorCode::ERR_HASH_IO_ERROR, "Root directory does not exist: " + root_dir_.string());
    }

    // Se for um arquivo isolado em vez de diretório
    if (!fs::is_directory(root_dir_)) {
        auto res = ComputeFileSha256(root_dir_);
        if (res.is_ok()) {
            deep_content_hash_ = res.value();
        }
        return res;
    }

    std::vector<std::string> rel_paths;
    std::error_code ec;

    for (const auto& entry : fs::recursive_directory_iterator(root_dir_, fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_regular_file() || entry.is_symlink()) {
            std::string rel = fs::relative(entry.path(), root_dir_, ec).string();
            if (!is_excluded(rel, exclusions)) {
                rel_paths.push_back(rel);
            }
        }
    }

    std::sort(rel_paths.begin(), rel_paths.end());

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    constexpr size_t buffer_size = 65536;
    std::vector<char> buffer(buffer_size);

    for (const auto& rel : rel_paths) {
        fs::path full_p = root_dir_ / rel;
        EVP_DigestUpdate(ctx, rel.data(), rel.size());

        if (fs::is_symlink(full_p, ec)) {
            std::string target = fs::read_symlink(full_p, ec).string();
            EVP_DigestUpdate(ctx, target.data(), target.size());
        } else {
            std::ifstream file(full_p, std::ios::binary);
            if (file.is_open()) {
                while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
                    EVP_DigestUpdate(ctx, buffer.data(), static_cast<size_t>(file.gcount()));
                }
            }
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    deep_content_hash_ = to_hex(digest, digest_len);
    return deep_content_hash_;
}

void InputHasher::AppendEnvironmentFlag(const std::string& key, const std::string& value) {
    env_flags_[key] = value;
}

void InputHasher::AppendToolchainChecksum(const std::string& tool_name, const std::string& sha256_hex) {
    toolchain_checksums_[tool_name] = sha256_hex;
}

CarbonResult<void> InputHasher::AppendExternalFile(const fs::path& file_path) {
    auto res = ComputeFileSha256(file_path);
    if (!res) return res.error();
    external_files_[file_path.string()] = res.value();
    return CarbonResult<void>::ok();
}

std::string InputHasher::FinalizeCompositeFingerprint() {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    // 1. Content Hash
    EVP_DigestUpdate(ctx, deep_content_hash_.data(), deep_content_hash_.size());

    // 2. Env flags (map is sorted by key automatically in std::map)
    for (const auto& [k, v] : env_flags_) {
        EVP_DigestUpdate(ctx, k.data(), k.size());
        EVP_DigestUpdate(ctx, "=", 1);
        EVP_DigestUpdate(ctx, v.data(), v.size());
        EVP_DigestUpdate(ctx, ";", 1);
    }

    // 3. Toolchains
    for (const auto& [tool, hash] : toolchain_checksums_) {
        EVP_DigestUpdate(ctx, tool.data(), tool.size());
        EVP_DigestUpdate(ctx, ":", 1);
        EVP_DigestUpdate(ctx, hash.data(), hash.size());
        EVP_DigestUpdate(ctx, ";", 1);
    }

    // 4. External files
    for (const auto& [file, hash] : external_files_) {
        EVP_DigestUpdate(ctx, file.data(), file.size());
        EVP_DigestUpdate(ctx, ":", 1);
        EVP_DigestUpdate(ctx, hash.data(), hash.size());
        EVP_DigestUpdate(ctx, ";", 1);
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    return to_hex(digest, digest_len);
}

} // namespace carbon::fingerprint
