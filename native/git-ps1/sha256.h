/*!
 * @brief SHA-256 utilities.
 * @author koturn
 * @file sha256.h
 */
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


//! String length of SHA-256.
static const size_t kSha256StrLength = 64;


int sha256str(char *pHashStr, const unsigned char *data, size_t dataSize);


#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */


#endif  /* SHA256_H */
