#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cstdint>
#include <memory>
#include "carbon/error.hpp"

namespace carbon {

namespace fs = std::filesystem;

inline std::string to_hex(const unsigned char* data, size_t len) {
    static const char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(hex_chars[(data[i] >> 4) & 0x0F]);
        result.push_back(hex_chars[data[i] & 0x0F]);
    }
    return result;
}

} // namespace carbon
