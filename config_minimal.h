/*
 * Minimal MbedTLS config for Classic Mac OS with Retro68 GCC toolchain
 * Only defines what we absolutely need
 */
#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* Include compatibility headers first */
#include "retro68_compat.h"

/* Core system support */
#define MBEDTLS_HAVE_INT64              /* GCC supports 64-bit integers */

/* Memory and platform */
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#define MBEDTLS_NO_UDBL_DIVISION

/* Essential crypto modules */
#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ERROR_C
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_RSA_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_VERSION_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C

/* Cipher modes */
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_CFB
#define MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_CIPHER_MODE_OFB
#define MBEDTLS_CIPHER_MODE_XTS

/* SSL/TLS settings */
#define MBEDTLS_SSL_MAX_CONTENT_LEN 8192
#define MBEDTLS_MPI_MAX_SIZE 512

/* Key exchange */
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

/* Elliptic curves */
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED

/* TLS versions - Only TLS 1.2 for Classic Mac OS compatibility */
#define MBEDTLS_SSL_PROTO_TLS1_2

/* RSA settings - Only PKCS#1 v1.5, no RSA-PSS for Classic Mac OS */
#define MBEDTLS_PKCS1_V15
#undef MBEDTLS_PKCS1_V21  /* Disable RSA-PSS padding */

/* Force TLS 1.2 maximum */
#define MBEDTLS_SSL_MAX_MAJOR_VERSION MBEDTLS_SSL_MAJOR_VERSION_3
#define MBEDTLS_SSL_MAX_MINOR_VERSION MBEDTLS_SSL_MINOR_VERSION_3

#endif /* MBEDTLS_CONFIG_H */