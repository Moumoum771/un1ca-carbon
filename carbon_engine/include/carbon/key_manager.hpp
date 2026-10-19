#pragma once
#include <string>
#include <filesystem>
#include <optional>
#include "carbon/common.hpp"

namespace carbon::security {

enum class KeyProfile : uint8_t {
    DEVELOPER_RELEASE_KEYS = 0,
    OFFICIAL_CARBON_KEYS = 1,
    AOSP_TEST_KEYS_FALLBACK = 2
};

struct KeyBundle {
    KeyProfile profile{KeyProfile::AOSP_TEST_KEYS_FALLBACK};
    fs::path platform_pk8;
    fs::path platform_pem;
    fs::path ota_pk8;
    fs::path ota_pem;
    std::string platform_der_hex;
};

class KeyManager {
public:
    explicit KeyManager(fs::path project_root);
    ~KeyManager();

    // Localiza as chaves ativas do projeto
    CarbonResult<KeyBundle> DiscoverActiveKeys(const std::optional<fs::path>& custom_keys_dir = std::nullopt);

    // Gera um novo par de chaves RSA 4096-bit no padrão Android
    CarbonResult<void> GenerateNewKeyPair(const fs::path& output_dir, 
                                         const std::string& org_name, 
                                         const std::string& common_name);

    // Injeta a assinatura da chave de plataforma no services.jar descompilado
    CarbonResult<void> InjectPlatformSignature(const fs::path& decoded_services_dir, 
                                              const std::string& platform_der_hex);

private:
    fs::path project_root_;
};

} // namespace carbon::security
