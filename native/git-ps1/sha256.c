/*!
 * @brief SHA-256 utilities.
 * @author koturn
 * @file sha256.c
 */
#include "sha256.h"
#include <stddef.h>
#include <string.h>
#include <openssl/sha.h>

#ifndef SHA256_DIGEST_LENGTH
#    define SHA256_DIGEST_LENGTH 32
#endif  // !defined(SHA256_DIGEST_LENGTH)


/*!
 * @brief Create SHA-256 string.
 *
 * @param [out] pHashStr  A destination pointer.
 * @param [in] data  Source data.
 * @param [in] dataSize  Source data size.
 *
 * @return Zero if success, otherwise non-zero.
 */
int sha256str(char *pHashStr, const unsigned char *data, size_t dataSize)
{
#ifdef OSSL_DEPRECATEDIN_3_0
    static const char hexDigits[] = "0123456789abcdef";

    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (SHA256(data, dataSize, hash) == NULL) {
        return 1;
    }

    size_t i = 0;
    for (i = 0; i < sizeof(hash) / sizeof(hash[0]); i++) {
        unsigned char b = hash[i];
        pHashStr[i * 2] = hexDigits[b >> 4];
        pHashStr[i * 2 + 1] = hexDigits[b & 0x0f];
    }
    pHashStr[i * 2] = '\0';
#else
    SHA256_CTX ctx;
    if (!SHA256_Init(&ctx)) {
        return 1;
    }
    if (!SHA256_Update(&ctx, data, dataSize)) {
        return 2;
    }
    if (SHA256_End(&ctx, sha256)) {
        return 3;
    }
#endif  // OSSL_DEPRECATEDIN_3_0

    return 0;
}
