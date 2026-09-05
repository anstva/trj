#include <stddef.h>
#include <stdint.h>

uint32_t H_0[8] = {
    0x6a09e667U,
    0xbb67ae85U,
    0x3c6ef372U,
    0xa54ff53aU,
    0x510e527fU,
    0x9b05688cU,
    0x1f83d9abU,
    0x5be0cd19U
};

uint32_t K[64] = {
    0x428a2f98U,
    0x71374491U,
    0xb5c0fbcfU,
    0xe9b5dba5U,
    0x3956c25bU,
    0x59f111f1U,
    0x923f82a4U,
    0xab1c5ed5U,
    0xd807aa98U,
    0x12835b01U,
    0x243185beU,
    0x550c7dc3U,
    0x72be5d74U,
    0x80deb1feU,
    0x9bdc06a7U,
    0xc19bf174U,
    0xe49b69c1U,
    0xefbe4786U,
    0x0fc19dc6U,
    0x240ca1ccU,
    0x2de92c6fU,
    0x4a7484aaU,
    0x5cb0a9dcU,
    0x76f988daU,
    0x983e5152U,
    0xa831c66dU,
    0xb00327c8U,
    0xbf597fc7U,
    0xc6e00bf3U,
    0xd5a79147U,
    0x06ca6351U,
    0x14292967U,
    0x27b70a85U,
    0x2e1b2138U,
    0x4d2c6dfcU,
    0x53380d13U,
    0x650a7354U,
    0x766a0abbU,
    0x81c2c92eU,
    0x92722c85U,
    0xa2bfe8a1U,
    0xa81a664bU,
    0xc24b8b70U,
    0xc76c51a3U,
    0xd192e819U,
    0xd6990624U,
    0xf40e3585U,
    0x106aa070U,
    0x19a4c116U,
    0x1e376c08U,
    0x2748774cU,
    0x34b0bcb5U,
    0x391c0cb3U,
    0x4ed8aa4aU,
    0x5b9cca4fU,
    0x682e6ff3U,
    0x748f82eeU,
    0x78a5636fU,
    0x84c87814U,
    0x8cc70208U,
    0x90befffaU,
    0xa4506cebU,
    0xbef9a3f7U,
    0xc67178f2U
};


static inline uint32_t rotl32(uint32_t x, unsigned int n)
{
    return (x << n) | (x >> (32 - n));
}

static inline uint32_t load_be32(const char *p, int offset)
{
    return ((uint32_t)p[offset]     << 24) |
           ((uint32_t)p[offset + 1] << 16) |
           ((uint32_t)p[offset + 2] <<  8) |
            (uint32_t)p[offset + 3];
}

static inline uint32_t swap32(uint32_t b)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(b);
#elif defined(_MSC_VER)
    return _byteswap_ulong(b);
#else
    return ((b & 0x000000FFU) << 24) |
           ((b & 0x0000FF00U) <<  8) |
           ((b & 0x00FF0000U) >>  8) |
           ((b & 0xFF000000U) >> 24);
#endif
}

void sha256_native(const uint32_t *message, uint32_t *digest)
{
    uint32_t W[64];
    uint32_t i;

    W[0] = load_be32((const char *)message, 0);
    W[1] = load_be32((const char *)message, 4);
    W[2] = load_be32((const char *)message, 8);
    W[3] = load_be32((const char *)message, 12);
    W[4] = load_be32((const char *)message, 16);
    W[5] = load_be32((const char *)message, 20);
    W[6] = load_be32((const char *)message, 24);
    W[7] = load_be32((const char *)message, 28);

    W[8]  = 0x80000000U;
    W[9]  = 0U;
    W[10] = 0U;
    W[11] = 0U;
    W[12] = 0U;
    W[13] = 0U;
    W[14] = 0U;
    W[15] = 256U;

    for (i = 16; i < 64; ++i)
    {
        uint32_t s0 = rotl32(W[i - 15],  (uint32_t)(32 - 7)) ^ rotl32(W[i - 15], (uint32_t)(32 - 18)) ^ (W[i - 15] >>  3);
        uint32_t s1 = rotl32(W[i -  2], (uint32_t)(32 - 17)) ^ rotl32(W[i -  2], (uint32_t)(32 - 19)) ^ (W[i -  2] >> 10);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    uint32_t a = H_0[0];
    uint32_t b = H_0[1];
    uint32_t c = H_0[2];
    uint32_t d = H_0[3];
    uint32_t e = H_0[4];
    uint32_t f = H_0[5];
    uint32_t g = H_0[6];
    uint32_t h = H_0[7];

    for (i = 0; i < 64; ++i)
    {
        uint32_t S1 = rotl32(e, (uint32_t)(32 - 6)) ^ rotl32(e, (uint32_t)(32 - 11)) ^ rotl32(e, (uint32_t)(32 - 25));
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t temporary1 = h + S1 + ch + K[i] + W[i];
        uint32_t S0 = rotl32(a, (uint32_t)(32 - 2)) ^ rotl32(a, (uint32_t)(32 - 13)) ^ rotl32(a, (uint32_t)(32 - 22));
        uint32_t major = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temporary2 = S0 + major;

        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;

        digest[0] = H_0[0] + a;
        digest[1] = H_0[1] + b;
        digest[2] = H_0[2] + c;
        digest[3] = H_0[3] + d;
        digest[4] = H_0[4] + e;
        digest[5] = H_0[5] + f;
        digest[6] = H_0[6] + g;
        digest[7] = H_0[7] + h;
    }
}
