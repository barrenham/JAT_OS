#include "../include/stdint.h"
#include "../include/stdio.h"
#include "../include/fs.h"
#include "../include/io.h"
#include "../include/crypto.h"
#include "../include/syscall.h"

#define SHA256_ROTL(a, b) (((a >> (32 - b)) & (0x7fffffff >> (31 - b))) | (a << b))
#define SHA256_SR(a, b) ((a >> b) & (0x7fffffff >> (b - 1)))
#define SHA256_Ch(x, y, z) ((x & y) ^ ((~x) & z))
#define SHA256_Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_E0(x) (SHA256_ROTL(x, 30) ^ SHA256_ROTL(x, 19) ^ SHA256_ROTL(x, 10))
#define SHA256_E1(x) (SHA256_ROTL(x, 26) ^ SHA256_ROTL(x, 21) ^ SHA256_ROTL(x, 7))
#define SHA256_O0(x) (SHA256_ROTL(x, 25) ^ SHA256_ROTL(x, 14) ^ SHA256_SR(x, 3))
#define SHA256_O1(x) (SHA256_ROTL(x, 15) ^ SHA256_ROTL(x, 13) ^ SHA256_SR(x, 10))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define BUFFER_SIZE 1024

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static void sha256_init(SHA256_CTX *ctx)
{
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

static void sha256_transform(SHA256_CTX *ctx, const unsigned char *data)
{
    uint32_t W[64];
    uint32_t A, B, C, D, E, F, G, H, T1, T2;
    int i;
    for (i = 0; i < 16; i++)
        W[i] = (data[i * 4] << 24) | (data[i * 4 + 1] << 16) |
               (data[i * 4 + 2] << 8) | (data[i * 4 + 3]);
    for (i = 16; i < 64; i++)
        W[i] = SHA256_O1(W[i - 2]) + W[i - 7] + SHA256_O0(W[i - 15]) + W[i - 16];
    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];
    F = ctx->state[5];
    G = ctx->state[6];
    H = ctx->state[7];
    for (i = 0; i < 64; i++)
    {
        T1 = H + SHA256_E1(E) + SHA256_Ch(E, F, G) + K[i] + W[i];
        T2 = SHA256_E0(A) + SHA256_Maj(A, B, C);
        H = G;
        G = F;
        F = E;
        E = D + T1;
        D = C;
        C = B;
        B = A;
        A = T1 + T2;
    }
    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
    ctx->state[5] += F;
    ctx->state[6] += G;
    ctx->state[7] += H;
}

void SHA256(const char *file_path, char sha[SHA256_DIGEST_SIZE])
{
    int flag = 0, size, hash_size;
    SHA256_CTX ctx;
    unsigned char *buffer;
    unsigned char digest[SHA256_DIGEST_SIZE];
    int bytes_read;
    int fd;
    sha256_init(&ctx);
    fd = openFile(file_path, O_RDONLY);
    flag = get_file_attr(fd);
    size = getfilesize(fd);
    hash_size = (flag & INODE_HASHED) ? size - SHA256_DIGEST_SIZE : size;
    if (fd < 0)
    {
        printf("Failed to open file\n");
        return;
    }
    buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        printf("Failed to allocate buffer\n");
        closeFile(fd);
        return NULL;
    }
    uint32_t total_read = 0;
    while ((bytes_read = read(fd, buffer,
                              MIN(BUFFER_SIZE, hash_size - total_read))) > 0)
    {
        int block_count = bytes_read / SHA256_BLOCK_SIZE;
        int i;
        for (i = 0; i < block_count; i++)
            sha256_transform(&ctx, buffer + (i * SHA256_BLOCK_SIZE));
        if (bytes_read % SHA256_BLOCK_SIZE)
            memcpy(ctx.buffer,
                   buffer + (block_count * SHA256_BLOCK_SIZE),
                   bytes_read % SHA256_BLOCK_SIZE);
        ctx.count += bytes_read;
        total_read += bytes_read;
        if (total_read >= hash_size)
            break;
    }
    closeFile(fd);
    int remaining = ctx.count % SHA256_BLOCK_SIZE;
    ctx.buffer[remaining] = 0x80;
    remaining++;
    if (remaining > SHA256_BLOCK_SIZE - 8)
    {
        memset(ctx.buffer + remaining, 0, SHA256_BLOCK_SIZE - remaining);
        sha256_transform(&ctx, ctx.buffer);
        memset(ctx.buffer, 0, SHA256_BLOCK_SIZE - 8);
    }
    else
        memset(ctx.buffer + remaining, 0, SHA256_BLOCK_SIZE - remaining - 8);
    uint64_t bits = ctx.count * 8;
    ctx.buffer[56] = (bits >> 56) & 0xff;
    ctx.buffer[57] = (bits >> 48) & 0xff;
    ctx.buffer[58] = (bits >> 40) & 0xff;
    ctx.buffer[59] = (bits >> 32) & 0xff;
    ctx.buffer[60] = (bits >> 24) & 0xff;
    ctx.buffer[61] = (bits >> 16) & 0xff;
    ctx.buffer[62] = (bits >> 8) & 0xff;
    ctx.buffer[63] = bits & 0xff;
    sha256_transform(&ctx, ctx.buffer);
    for (int i = 0; i < 8; i++)
    {
        digest[i * 4] = (ctx.state[i] >> 24) & 0xff;
        digest[i * 4 + 1] = (ctx.state[i] >> 16) & 0xff;
        digest[i * 4 + 2] = (ctx.state[i] >> 8) & 0xff;
        digest[i * 4 + 3] = ctx.state[i] & 0xff;
    }
    // for (int i = 0; i < SHA256_DIGEST_SIZE; i++)
    //     sprintf(sha + i * 2, "%02x", digest[i]);
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++)
        sha[i] = digest[i];
    // printf("SHA256: %s\n", sha);
    free(buffer);
}