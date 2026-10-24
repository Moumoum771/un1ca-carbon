#pragma once
#include <openssl/sha.h>
#include <cstdint>
#include <cstddef>

typedef SHA256_CTX AvbSHA256Ctx;
static inline void avb_sha256_init(AvbSHA256Ctx* ctx) { SHA256_Init(ctx); }
static inline void avb_sha256_update(AvbSHA256Ctx* ctx, const void* data, size_t len) { SHA256_Update(ctx, data, len); }
static inline uint8_t* avb_sha256_final(AvbSHA256Ctx* ctx) {
    static uint8_t md[32];
    SHA256_Final(md, ctx);
    return md;
}
