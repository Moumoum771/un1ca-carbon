#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "carbon/common.hpp"

namespace carbon::fingerprint {

struct StatRecord {
    std::string relative_path;
    uint64_t size_bytes;
    int64_t  mtime_ns;
    uint32_t mode;
};

class InputHasher {
public:
    explicit InputHasher(fs::path root_dir);
    ~InputHasher();

    // Hashing rápido de estatísticas de arquivos (Tier 1: stat-cache)
    CarbonResult<std::string> ComputeStatHash(const std::vector<std::string>& exclusions = {});

    // Hashing criptográfico profundo de conteúdo (Tier 2: SHA-256)
    CarbonResult<std::string> ComputeDeepContentHash(const std::vector<std::string>& exclusions = {});

    // Adiciona flags de linha de comando ou variáveis de ambiente ao hash composto
    void AppendEnvironmentFlag(const std::string& key, const std::string& value);

    // Adiciona identificador ou checksum da ferramenta de compilação
    void AppendToolchainChecksum(const std::string& tool_name, const std::string& sha256_hex);

    // Adiciona caminhos adicionais (ex: patches avulsos ou scripts externos)
    CarbonResult<void> AppendExternalFile(const fs::path& file_path);

    // Retorna a digital final composta (SHA-256)
    std::string FinalizeCompositeFingerprint();

    // Utilitário para calcular SHA-256 de um arquivo isolado
    static CarbonResult<std::string> ComputeFileSha256(const fs::path& file_path);

private:
    fs::path root_dir_;
    std::string deep_content_hash_;
    std::map<std::string, std::string> env_flags_;
    std::map<std::string, std::string> toolchain_checksums_;
    std::map<std::string, std::string> external_files_;
};

} // namespace carbon::fingerprint
