#include <metal_stdlib>

using namespace metal;

constant uint H_0[8] = {
    0x6a09e667U,
    0xbb67ae85U,
    0x3c6ef372U,
    0xa54ff53aU,
    0x510e527fU,
    0x9b05688cU,
    0x1f83d9abU,
    0x5be0cd19U
};

constant uint K[64] = {
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

inline uint load_be32(device const uchar *p, int offset)
{
    return ((uint)p[offset]     << 24) |
           ((uint)p[offset + 1] << 16) |
           ((uint)p[offset + 2] <<  8) |
            (uint)p[offset + 3];
}

inline uint rotate(uint b, uint n)
{
    return (b << (n & 31)) | (b >> ((32 - n) & 31));
}

inline uint swap32(uint b)
{
    return rotate(b & 0x00FF00FFU, 24U) | rotate(b & 0xFF00FF00U, 8U);
}

kernel void sha256_metal_kernel(
    device const uint *message [[buffer(0)]],
    device uint       *digest  [[buffer(1)]],
    uint               gid     [[thread_position_in_grid]]
)
{
    device const uint *msg = message + gid * 8;
    device uint *out       = digest + gid * 8;

    uint W[64];
    uint i;

    W[0] = load_be32((device const uchar *)msg, 0);
    W[1] = load_be32((device const uchar *)msg, 4);
    W[2] = load_be32((device const uchar *)msg, 8);
    W[3] = load_be32((device const uchar *)msg, 12);
    W[4] = load_be32((device const uchar *)msg, 16);
    W[5] = load_be32((device const uchar *)msg, 20);
    W[6] = load_be32((device const uchar *)msg, 24);
    W[7] = load_be32((device const uchar *)msg, 28);

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
        uint s0 = rotate(W[i - 15],  (uint)(32 - 7)) ^ rotate(W[i - 15], (uint)(32 - 18)) ^ (W[i - 15] >>  3);
        uint s1 = rotate(W[i -  2], (uint)(32 - 17)) ^ rotate(W[i -  2], (uint)(32 - 19)) ^ (W[i -  2] >> 10);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    uint a = H_0[0];
    uint b = H_0[1];
    uint c = H_0[2];
    uint d = H_0[3];
    uint e = H_0[4];
    uint f = H_0[5];
    uint g = H_0[6];
    uint h = H_0[7];

    for (i = 0; i < 64; ++i)
    {
        uint S1 = rotate(e, (uint)(32 - 6)) ^ rotate(e, (uint)(32 - 11)) ^ rotate(e, (uint)(32 - 25));
        uint ch = (e & f) ^ (~e & g);
        uint temporary1 = h + S1 + ch + K[i] + W[i];
        uint S0 = rotate(a, (uint)(32 - 2)) ^ rotate(a, (uint)(32 - 13)) ^ rotate(a, (uint)(32 - 22));
        uint major = (a & b) ^ (a & c) ^ (b & c);
        uint temporary2 = S0 + major;

        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }

    out[0] = H_0[0] + a;
    out[1] = H_0[1] + b;
    out[2] = H_0[2] + c;
    out[3] = H_0[3] + d;
    out[4] = H_0[4] + e;
    out[5] = H_0[5] + f;
    out[6] = H_0[6] + g;
    out[7] = H_0[7] + h;
}
