#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "stdint.h"

void SM4_KeySchedule(uint8_t MK[16], uint32_t rk[32]);
int SM4_ECB_Encrypt(uint8_t *data, uint32_t dataLen, uint32_t bufferSize, uint32_t *outputLen, const uint32_t rk[32]);
int SM4_ECB_Decrypt(uint8_t *data, uint32_t dataLen, uint32_t *outputLen, const uint32_t rk[32]);

#endif