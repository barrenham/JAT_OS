#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "stdint.h"

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

typedef struct
{
    uint32_t state[8];
    uint64_t count;
    unsigned char buffer[SHA256_BLOCK_SIZE];
} SHA256_CTX;

void SM4_KeySchedule(uint8_t MK[16], uint32_t rk[32]);
int SM4_ECB_Encrypt(uint8_t *data, uint32_t dataLen, uint32_t bufferSize, uint32_t *outputLen, const uint32_t rk[32]);
int SM4_ECB_Decrypt(uint8_t *data, uint32_t dataLen, uint32_t *outputLen, const uint32_t rk[32]);
void encrypt_data(uint8_t *data, uint32_t size, const char *key);
void decrypt_data(uint8_t *data, uint32_t size, const char *key);
void SHA256(const char *file_path, char sha[SHA256_DIGEST_SIZE]);

#endif