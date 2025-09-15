/*
 * cipher_stubs.c
 *
 * Stub implementations for disabled MbedTLS cipher algorithms
 * (GCM, ChaCha20, ChachaPoly) that are referenced by cipher.c but disabled
 */

#include "mbedtls/cipher.h"
#include "mbedtls/error.h"

/* GCM stubs */
void mbedtls_gcm_init(void *ctx) {
    (void)ctx;
}

void mbedtls_gcm_free(void *ctx) {
    (void)ctx;
}

int mbedtls_gcm_setkey(void *ctx, int cipher, const unsigned char *key, unsigned int keybits) {
    (void)ctx; (void)cipher; (void)key; (void)keybits;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_gcm_crypt_and_tag(void *ctx, int mode, size_t length,
                             const unsigned char *iv, size_t iv_len,
                             const unsigned char *add, size_t add_len,
                             const unsigned char *input, unsigned char *output,
                             size_t tag_len, unsigned char *tag) {
    (void)ctx; (void)mode; (void)length; (void)iv; (void)iv_len;
    (void)add; (void)add_len; (void)input; (void)output; (void)tag_len; (void)tag;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_gcm_auth_decrypt(void *ctx, size_t length,
                            const unsigned char *iv, size_t iv_len,
                            const unsigned char *add, size_t add_len,
                            const unsigned char *tag, size_t tag_len,
                            const unsigned char *input, unsigned char *output) {
    (void)ctx; (void)length; (void)iv; (void)iv_len; (void)add; (void)add_len;
    (void)tag; (void)tag_len; (void)input; (void)output;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_gcm_update(void *ctx, size_t length,
                      const unsigned char *input, unsigned char *output) {
    (void)ctx; (void)length; (void)input; (void)output;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

/* ChaCha20 stubs */
void mbedtls_chacha20_init(void *ctx) {
    (void)ctx;
}

void mbedtls_chacha20_free(void *ctx) {
    (void)ctx;
}

int mbedtls_chacha20_setkey(void *ctx, const unsigned char key[32]) {
    (void)ctx; (void)key;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_chacha20_starts(void *ctx, const unsigned char nonce[12], unsigned int counter) {
    (void)ctx; (void)nonce; (void)counter;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_chacha20_update(void *ctx, size_t size,
                           const unsigned char *input, unsigned char *output) {
    (void)ctx; (void)size; (void)input; (void)output;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

/* ChachaPoly stubs */
void mbedtls_chachapoly_init(void *ctx) {
    (void)ctx;
}

void mbedtls_chachapoly_free(void *ctx) {
    (void)ctx;
}

int mbedtls_chachapoly_setkey(void *ctx, const unsigned char key[32]) {
    (void)ctx; (void)key;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_chachapoly_encrypt_and_tag(void *ctx, size_t length,
                                      const unsigned char nonce[12],
                                      const unsigned char *aad, size_t aad_len,
                                      const unsigned char *input, unsigned char *output,
                                      unsigned char tag[16]) {
    (void)ctx; (void)length; (void)nonce; (void)aad; (void)aad_len;
    (void)input; (void)output; (void)tag;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_chachapoly_auth_decrypt(void *ctx, size_t length,
                                   const unsigned char nonce[12],
                                   const unsigned char *aad, size_t aad_len,
                                   const unsigned char tag[16],
                                   const unsigned char *input, unsigned char *output) {
    (void)ctx; (void)length; (void)nonce; (void)aad; (void)aad_len;
    (void)tag; (void)input; (void)output;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}

int mbedtls_chachapoly_update(void *ctx, size_t len,
                             const unsigned char *input, unsigned char *output) {
    (void)ctx; (void)len; (void)input; (void)output;
    return MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
}