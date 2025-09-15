/*
 * SSLWrapper.h
 *
 * Simplified SSL wrapper interface for Retro68 port
 */
#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <OpenTransport.h>
#include <OpenTptInternet.h>
#include <TextEdit.h>  /* For TEHandle */

/* Include the necessary mbedTLS headers */
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

typedef void (*LoggingCallback)(const char* message);

/* SSL State Structure */
typedef struct {
    EndpointRef              endpoint;      /* OT endpoint for the connection */
    mbedtls_ssl_context      ssl;           /* mbedTLS context */
    mbedtls_ssl_config       conf;          /* mbedTLS configuration */
    mbedtls_entropy_context  entropy;       /* Entropy source for SSL */
    mbedtls_ctr_drbg_context ctr_drbg;      /* Random number generator context */
    mbedtls_x509_crt         cacert;        /* CA certificates */
    mbedtls_x509_crt         clicert;       /* Client certificate */
    mbedtls_pk_context       pkey;          /* Client private key */
    int                      initialized;   /* Flag to track initialization */
    unsigned char            *large_buffer; /* Pre-allocated large buffer */
    size_t                   buffer_size;   /* Size of the buffer */
} SSLState;

/* Function Prototypes */
OSStatus SSL_Initialize(SSLState* state, LoggingCallback logFunc);
OSStatus SSL_Connect(SSLState* state, InetAddress* address, TEHandle responseText, LoggingCallback logFunc);
OSStatus SSL_Send(SSLState* state, const void* buffer, size_t length, size_t* bytesSent, LoggingCallback logFunc);
OSStatus SSL_Receive(SSLState* state, void* buffer, size_t bufferSize, size_t* bytesReceived, LoggingCallback logFunc);
void SSL_Close(SSLState* state);
int load_root_ca_cert(SSLState* state, const char* ca_cert_pem, size_t ca_cert_len, LoggingCallback logFunc);

#endif /* SSL_WRAPPER_H */