/* 
 * SSLWrapper.h
 * 
 * SSL wrapper interface for Classic Mac OS using PolarSSL/mbedTLS
 */

#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <OpenTransport.h>
#include <OpenTptInternet.h>

/* Include the necessary mbedTLS headers */
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/memory_buffer_alloc.h"

typedef void (*LoggingCallback)(const char* message);

/* SSL State Structure */
typedef struct {
    EndpointRef              endpoint;      /* OT endpoint for the connection */
    mbedtls_ssl_context      ssl;           /* PolarSSL context */
    mbedtls_ssl_config       conf;          /* PolarSSL configuration */
    mbedtls_entropy_context  entropy;       /* Entropy source for SSL */
    mbedtls_ctr_drbg_context ctr_drbg;      /* Random number generator context */
    mbedtls_x509_crt		 cacert;		/* CA certificates */
    mbedtls_x509_crt		 clicert;		/* Client certificate */
    mbedtls_pk_context		 pkey;			/* Client private key */
    int                      initialized;   /* Flag to track initialization */
    unsigned char			 *large_buffer; /* Pre-allocated large buffer */
    size_t					 buffer_size;   /* Size of the buffer */
} SSLState;

static unsigned char heap_buf[65536];




/* Function Prototypes */
static unsigned long GetPPCTimer(bool is601);
static double GetTimeBaseResolution(void);
int mac_entropy_gather(unsigned char *output, size_t len);
int mac_entropy_func(void *data, unsigned char *output, size_t len);
OSStatus SSL_Initialize(SSLState* state, LoggingCallback logFunc);
OSStatus SSL_Connect(SSLState* state, InetAddress* address, TEHandle responseText, LoggingCallback logFunc);
OSStatus SSL_Send(SSLState* state, const void* buffer, size_t length, size_t* bytesSent, LoggingCallback logFunc);
OSStatus SSL_Receive(SSLState* state, void* buffer, size_t bufferSize, size_t* bytesReceived, LoggingCallback logFunc);
void SSL_Close(SSLState* state);
int mac_entropy_func(void *data, unsigned char *ouput, size_t len);
static void ssl_debug_callback(void *ctx, int level, const char *file, int line, const char *str);
int load_root_ca_cert(SSLState* state, const char* ca_cert_pem, size_t ca_cert_len, LoggingCallback logFunc);
int ssl_verify_callback(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags);
int custom_ssl_conf_dh_param(mbedtls_ssl_config *conf, mbedtls_mpi *P, mbedtls_mpi *G);

#endif /* SSL_WRAPPER_H */