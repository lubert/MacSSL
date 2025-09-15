/*
 * MbedTLS config for Classic Mac OS with Retro68 GCC toolchain
 * Adapted from CodeWarrior version to use GCC capabilities
 */
#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* We're using Retro68, not CodeWarrior */
#define RETRO68_BUILD 1

/* Include compatibility headers first */
#include "retro68_compat.h"

/* System support - be explicit about what we enable/disable */
/* DON'T define MBEDTLS_HAVE_ASM - conflicts with MBEDTLS_HAVE_INT64 */
#undef MBEDTLS_HAVE_TIME      /* Explicitly disable time functions */
#undef MBEDTLS_ENTROPY_PLATFORM
#undef MBEDTLS_FS_IO
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_HAVEGE_C

/* GCC supports 64-bit integers, unlike CodeWarrior */
#define MBEDTLS_HAVE_INT64

/* Use the 'portable c' implementation of multi-precision arithmetic */
#define MBEDTLS_NO_UDBL_DIVISION

/* Disable threading - Classic Mac OS doesn't have POSIX threads */
#undef MBEDTLS_THREADING_C
#undef MBEDTLS_THREADING_PTHREAD

/* Disable features we don't need */
#undef MBEDTLS_ZLIB_SUPPORT
#undef MBEDTLS_PKCS11_C
#undef MBEDTLS_PKCS12_C
#undef MBEDTLS_CMAC_C
#undef MBEDTLS_NET_C  /* We'll implement our own using Open Transport */
#undef MBEDTLS_SSL_CACHE_C
#undef MBEDTLS_SSL_SESSION_TICKETS
#undef MBEDTLS_XTEA_C

/* Reduce memory requirements */
#define MBEDTLS_SSL_MAX_CONTENT_LEN 8192
#define MBEDTLS_MPI_MAX_SIZE 512

/* Core modules */
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_PROTO_TLS1
#define MBEDTLS_SSL_PROTO_TLS1_1
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_SSL3
#define MBEDTLS_SSL_RENEGOTIATION

/* Server Name Indication */
#define MBEDTLS_SSL_SERVER_NAME_INDICATION

/* Debug support */
#define MBEDTLS_DEBUG_C
#define MBEDTLS_ERROR_C

/* Crypto algorithms - compatible with server */
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_C
#define MBEDTLS_DHM_C
#define MBEDTLS_RSA_C
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_X509_CRT_PARSE_C

/* Elliptic curves */
#define MBEDTLS_SSL_ECP_RESTARTABLE
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED

/* Key exchange algorithms */
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

/* Modern cipher support - disable problematic ones for now */
#undef MBEDTLS_GCM_C
#undef MBEDTLS_CIPHER_MODE_GCM
#undef MBEDTLS_CHACHA20_C
#undef MBEDTLS_POLY1305_C
#undef MBEDTLS_CHACHAPOLY_C

/* RSA support */
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C

/* Certificate handling */
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C

/* PK layer */
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C

/* Entropy and random number generation */
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C

/* Base64 encoding/decoding */
#define MBEDTLS_BASE64_C

/* PEM parsing */
#define MBEDTLS_PEM_PARSE_C

/* Platform abstraction layer */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO mbedtls_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO   mbedtls_free

/* GCC-specific optimizations */
#ifdef __GNUC__
#define MBEDTLS_OPTIMIZE_TININESS
#endif

/* Custom entropy function for Mac OS */
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#endif /* MBEDTLS_CONFIG_H */