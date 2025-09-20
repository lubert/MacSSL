/*
 * SSLWrapper_retro68.c
 *
 * SSL wrapper implementation for Classic Mac OS using MbedTLS
 * Ported from CodeWarrior to Retro68 GCC toolchain
 */

#include "retro68_compat.h"
#include "ssl_wrapper.h"
#include <string.h>
#include <stdio.h>
#include <Memory.h>
#include <Resources.h>
#include <Processes.h>
#include <Folders.h>
#include <Files.h>
#include <Gestalt.h>
#include <LowMem.h>
/* MbedTLS headers with proper path */
#include "mbedtls/debug.h"
#include "mbedtls/ssl_ciphersuites.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/platform.h"
/* Application headers */
#include "api.h"
#include "logging.h"

// Root CA certificates (Let's Encrypt)
const char *ca_cert_pem =
"-----BEGIN CERTIFICATE-----\r\n"
"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\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN CERTIFICATE-----\r\n"
"MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw\r\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\r\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw\r\n"
"WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\r\n"
"RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\r\n"
"AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP\r\n"
"R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx\r\n"
"sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm\r\n"
"NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg\r\n"
"Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG\r\n"
"/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC\r\n"
"AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB\r\n"
"Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA\r\n"
"FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw\r\n"
"AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw\r\n"
"Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB\r\n"
"gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W\r\n"
"PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl\r\n"
"ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz\r\n"
"CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm\r\n"
"lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4\r\n"
"avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2\r\n"
"yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O\r\n"
"yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids\r\n"
"hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+\r\n"
"HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv\r\n"
"MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX\r\n"
"nLRbwHOoq7hHwg==\r\n"
"-----END CERTIFICATE-----\r\n";

/* Signature algorithms to use - from strongest to weakest */
static const int sig_algs[] = {
    0x0403, /* SHA-256 + RSA */
    0x0503, /* SHA-384 + RSA */
    0x0203, /* SHA-1 + RSA */
    0
};

static unsigned char heap_buf[65536];

/* External entropy function (implemented in mac_entropy.c) */
extern int mac_entropy_func(void *data, unsigned char *output, size_t len);

/* Certificate verification callback for debugging */
static int cert_verify_callback(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    LoggingCallback logFunc = (LoggingCallback)data;
    char buf[1024];
    char msg[1200];

    if (logFunc) {
        sprintf(msg, "Certificate verification callback: depth=%d, flags=0x%08lx", depth, (unsigned long)*flags);
        logFunc(msg);

        /* Get certificate subject */
        if (mbedtls_x509_dn_gets(buf, sizeof(buf), &crt->subject) > 0) {
            sprintf(msg, "  Certificate subject: %s", buf);
            logFunc(msg);
        }

        /* Get certificate issuer */
        if (mbedtls_x509_dn_gets(buf, sizeof(buf), &crt->issuer) > 0) {
            sprintf(msg, "  Certificate issuer: %s", buf);
            logFunc(msg);
        }

        /* Check specific verification flags */
        if (*flags & MBEDTLS_X509_BADCERT_EXPIRED) {
            logFunc("  ERROR: Certificate has expired");
        }
        if (*flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
            logFunc("  ERROR: Certificate is not trusted (CA verification failed)");
        }
        if (*flags & MBEDTLS_X509_BADCERT_CN_MISMATCH) {
            logFunc("  ERROR: Certificate CN does not match hostname");
        }
        if (*flags & MBEDTLS_X509_BADCERT_FUTURE) {
            logFunc("  ERROR: Certificate validity starts in the future");
        }
        if (*flags & MBEDTLS_X509_BADCERT_BAD_MD) {
            logFunc("  ERROR: Certificate signed with unacceptable hash");
        }
        if (*flags & MBEDTLS_X509_BADCERT_BAD_PK) {
            logFunc("  ERROR: Certificate signed with unacceptable PK algorithm");
        }
        if (*flags & MBEDTLS_X509_BADCERT_BAD_KEY) {
            logFunc("  ERROR: Certificate signed with unacceptable key");
        }
    }

    /* Return 0 to continue verification, non-zero to fail */
    return 0;
}

/*
 * Custom send/receive functions for MbedTLS that use Open Transport
 */
static int ot_send(void *ctx, const unsigned char *buf, size_t len)
{
    EndpointRef endpoint = (EndpointRef)ctx;
    OTResult result;

    result = OTSnd(endpoint, (void*)buf, len, 0);

    if (result >= 0)
        return result;
    else
        return -1; /* Return generic error code */
}

static int ot_recv(void *ctx, unsigned char *buf, size_t len)
{
    EndpointRef endpoint = (EndpointRef)ctx;
    OTResult result;

    result = OTRcv(endpoint, buf, len, 0);

    if (result > 0)
        return result;
    else if (result == 0)
        return 0; /* Connection closed */
    else if (result == kOTNoDataErr)
        return MBEDTLS_ERR_SSL_WANT_READ; /* No data available right now */
    else
        return -1; /* Other error */
}

/* Initialize the SSL state with progressive logging via callback */
OSStatus SSL_Initialize(SSLState* state, LoggingCallback logFunc)
{
    int ret;
    UInt32 ticks;
    char errorMsg[100];
    unsigned char enhanced_seed[128];
    UInt32 fallback_seed[8];
    Point mouse_point;

    /* Supported elliptic curves in order of preference */
    static mbedtls_ecp_group_id curve_list[] = {
        MBEDTLS_ECP_DP_SECP256R1,
        MBEDTLS_ECP_DP_NONE
    };

    /* Set cipher suites that are most likely to be compatible */
    static const int ciphersuites[] = {
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,  /* Modern ECDHE with AES-CBC */
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,  /* ECDHE with AES-128-CBC */
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,     /* ECDHE with AES-256-CBC */
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,     /* ECDHE with AES-128-CBC */
        MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256,        /* RSA fallback with SHA-256 */
        MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256,        /* RSA fallback with SHA-256 */
        MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA,           /* RSA fallback with SHA-1 */
        MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,           /* RSA fallback with SHA-1 */
        0  /* Terminator */
    };

    if (logFunc) logFunc("Starting SSL initialization");

    if (state == NULL) {
        if (logFunc) logFunc("Error: NULL state pointer");
        return paramErr;
    }

    /* If already initialized, clean up first */
    if (state->initialized) {
        if (logFunc) logFunc("Found existing SSL state, cleaning up...");
        SSL_Close(state);
    }

    /* Clear the state structure */
    memset(state, 0, sizeof(SSLState));

    /* Allocate the large buffer for handshake operations */
    state->buffer_size = 16384; /* 16k should be enough */
    state->large_buffer = (unsigned char*)mac_calloc(1, state->buffer_size);
    if(state->large_buffer == NULL) {
        if(logFunc) logFunc("Error: Failed to allocate large buffer");
        return memFullErr;
    }

    /* Set up the custom calloc and free for mbedtls internal use */
    mbedtls_platform_set_calloc_free(mac_calloc, mac_free);

    /* Set up the heap buffer */
    mbedtls_memory_buffer_alloc_init(heap_buf, sizeof(heap_buf));

    /* STEP 1: Gather entropy */
    if (logFunc) logFunc("Step 1: Gathering entropy sources");

    /* Create a better entropy source using Classic Mac resources */
    ticks = TickCount(); /* Get current tick count from Mac OS */

    /* Format info message */
    sprintf(errorMsg, "  System ticks: %lu", (unsigned long)ticks);
    if (logFunc) logFunc(errorMsg);

    /* STEP 2: Initialize entropy context */
    if (logFunc) logFunc("Step 2: Initializing entropy context");

    /* Initialize entropy source */
    mbedtls_entropy_init(&state->entropy);
    if (logFunc) logFunc("  Entropy context initialized");

    /* STEP 3: Initialize random number generator */
    if (logFunc) logFunc("Step 3: Initializing random number generator");

    /* Initialize the RNG context */
    mbedtls_ctr_drbg_init(&state->ctr_drbg);
    if (logFunc) logFunc("  RNG context initialized");

    /* STEP 4: Seed the RNG with enhanced entropy sources */
    if (logFunc) logFunc("Step 4: Seeding random number generator with enhanced entropy");

    /* Fill the buffer with entropy from our custom gatherer */
    mac_entropy_func(NULL, enhanced_seed, sizeof(enhanced_seed));

    /* Try seeding with our custom entropy function */
    ret = mbedtls_ctr_drbg_seed(
        &state->ctr_drbg,
        mac_entropy_func,       /* Use our custom entropy function */
        NULL,                   /* No context needed */
        enhanced_seed,          /* Additional seed data */
        sizeof(enhanced_seed)   /* Size of additional seed */
    );

    if (ret != 0) {
        /* If that fails, try a deterministic approach for testing ONLY */
        if (logFunc) {
            sprintf(errorMsg, "RNG seed failed (code %d). Trying fallback method...", ret);
            logFunc(errorMsg);
        }

        /* Use a mix of system values as a fallback seed */
        mouse_point = LMGetMouseLocation();
        fallback_seed[0] = TickCount();
        GetDateTime((unsigned long*)&fallback_seed[1]);
        fallback_seed[2] = (UInt32)((mouse_point.h <<16) | (mouse_point.v & 0xFFFF));
        fallback_seed[3] = (UInt32)CurResFile();
        fallback_seed[4] = (UInt32)LMGetCurApName();
        fallback_seed[5] = (UInt32)LMGetApFontID();

        /* Try the fallback approach */
        if (logFunc) logFunc("  Using fallback seeding method");

        ret = mbedtls_ctr_drbg_seed(
            &state->ctr_drbg,
            mbedtls_entropy_func,    /* Standard function */
            &state->entropy,         /* With our entropy context */
            (unsigned char*)fallback_seed,
            sizeof(fallback_seed)
        );
    }
    if (logFunc) logFunc("  RNG successfully seeded");

    /* STEP 5: Initialize SSL context */
    if (logFunc) logFunc("Step 5: Initializing SSL context");

    /* Initialize SSL context */
    mbedtls_ssl_init(&state->ssl);
    if (logFunc) logFunc("  SSL context initialized");

    /* STEP 6: Initialize SSL config */
    if (logFunc) logFunc("Step 6: Initializing SSL configuration");

    /* Initialize SSL config */
    mbedtls_ssl_config_init(&state->conf);
    if (logFunc) logFunc("  SSL configuration initialized");

    mbedtls_x509_crt_init(&state->cacert);
    mbedtls_x509_crt_init(&state->clicert);
    mbedtls_pk_init(&state->pkey);

    /* STEP 7: Set SSL config defaults */
    if (logFunc) logFunc("Step 7: Setting SSL configuration defaults");

    /* Set default SSL configuration */
    ret = mbedtls_ssl_config_defaults(
        &state->conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );

    if (ret != 0) {
        sprintf(errorMsg, "ERROR: SSL config defaults failed with code %d", ret);
        if (logFunc) logFunc(errorMsg);
        mbedtls_ssl_free(&state->ssl);
        mbedtls_ssl_config_free(&state->conf);
        mbedtls_ctr_drbg_free(&state->ctr_drbg);
        mbedtls_entropy_free(&state->entropy);
        return ret;
    }
    if (logFunc) logFunc("  SSL configuration defaults set");

    /* STEP 8: Configure SSL settings */
    if (logFunc) logFunc("Step 8: Configuring SSL settings");

    /* Configure ciphersuites */
    mbedtls_ssl_conf_ciphersuites(&state->conf, ciphersuites);

    /* Set RNG function */
    mbedtls_ssl_conf_rng(&state->conf, mbedtls_ctr_drbg_random, &state->ctr_drbg);
    if (logFunc) logFunc("  RNG function configured");

    /* Load the root CA certificate */
    ret = load_root_ca_cert(state, ca_cert_pem, strlen(ca_cert_pem), logFunc);
    if(ret != 0) {
        if(logFunc) logFunc("Warning: Failed to load CA certificates");
    }

    /* Set certificate verification callback for debugging */
    mbedtls_ssl_conf_verify(&state->conf, cert_verify_callback, logFunc);

    /* Set certificate verification mode to OPTIONAL for debugging */
    mbedtls_ssl_conf_authmode(&state->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    if (logFunc) logFunc("Certificate verification OPTIONAL (debugging mode)");

    /* Set TLS 1.2 for modern server compatibility */
    mbedtls_ssl_conf_min_version(&state->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&state->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    if (logFunc) logFunc("  SSL/TLS version TLS 1.2");

    /* Configure elliptic curves */
    mbedtls_ssl_conf_curves(&state->conf, curve_list);

    /* Increase timeout for slow connections */
    mbedtls_ssl_conf_read_timeout(&state->conf, 120000);
    if (logFunc) logFunc("  Read timeout set to 120 seconds");

    /* STEP 9: Setup SSL with configuration */
    if (logFunc) logFunc("Step 9: Applying configuration to SSL context");

    /* Setup SSL with config */
    ret = mbedtls_ssl_setup(&state->ssl, &state->conf);
    if (ret != 0) {
        sprintf(errorMsg, "ERROR: SSL setup failed with code %d", ret);
        if (logFunc) logFunc(errorMsg);
        mbedtls_ssl_config_free(&state->conf);
        mbedtls_ctr_drbg_free(&state->ctr_drbg);
        mbedtls_entropy_free(&state->entropy);
        return ret;
    }
    if (logFunc) logFunc("  SSL configuration successfully applied");

    /* Success! */
    state->initialized = 1;
    state->endpoint = kOTInvalidEndpointRef;

    if (logFunc) logFunc("SSL initialization completed successfully!");
    return noErr;
}

/* Create an endpoint for SSL communication */
static OSStatus CreateSecureEndpoint(EndpointRef* endpoint)
{
    OSStatus err;

    /* Create an endpoint using TCP protocol */
    *endpoint = OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, NULL, &err);
    return err;
}

/* Establish an SSL connection to the specified address */
OSStatus SSL_Connect(SSLState* state, InetAddress* address, const char* hostname, TEHandle responseText, LoggingCallback logFunc)
{
    OSStatus err;
    OTResult result;
    TBind bindReq;
    TCall sndCall;
    int ret;
    int handshake_attempts;
    char debug_msg[150];
    char msg[100];
    const char* cipher;
    const char* ver;

    if (!state->initialized)
        return paramErr;

    if (logFunc) logFunc("Setting Up SSL Connection...");

    /* Create the TCP endpoint */
    err = CreateSecureEndpoint(&state->endpoint);
    if (err != noErr) {
        sprintf(debug_msg, "Failed to create secure endpoint: %d", (int)err);
        if(logFunc) logFunc(debug_msg);
        return err;
    }

    if(logFunc) logFunc("Setting up SSL connection...");

    /* Bind the endpoint */
    bindReq.addr.maxlen = 0;
    bindReq.addr.len = 0;
    bindReq.addr.buf = NULL;
    bindReq.qlen = 0;

    err = OTBind(state->endpoint, &bindReq, NULL);
    if (err != noErr) {
        sprintf(debug_msg, "Failed to bind endpoint: %d", (int)err);
        if(logFunc) logFunc(debug_msg);
        OTCloseProvider(state->endpoint);
        state->endpoint = kOTInvalidEndpointRef;
        return err;
    }

    /* Set up the connection call structure */
    sndCall.addr.maxlen = sizeof(InetAddress);
    sndCall.addr.len = sizeof(InetAddress);
    sndCall.addr.buf = (UInt8*)address;

    sndCall.opt.maxlen = 0;
    sndCall.opt.len = 0;
    sndCall.opt.buf = NULL;

    sndCall.udata.maxlen = 0;
    sndCall.udata.len = 0;
    sndCall.udata.buf = NULL;

    /* Connect to the server */
    if(logFunc) logFunc("Establishing TCP connection...");

    err = OTConnect(state->endpoint, &sndCall, NULL);

    /* Check for asynchronous completion */
    if (err == kOTNoDataErr) {
        if(logFunc) logFunc("Waiting for connection completion...");

        result = OTLook(state->endpoint);

        if (result == T_CONNECT) {
            /* Accept the connection and finish connecting */
            err = OTRcvConnect(state->endpoint, NULL);
            if (err != noErr) {
                sprintf(debug_msg, "TCP connection failed during completion: %d", (int)err);
                if(logFunc) logFunc(debug_msg);
                OTCloseProvider(state->endpoint);
                state->endpoint = kOTInvalidEndpointRef;
                return err;
            }
        } else {
            sprintf(debug_msg, "Unexpected connection state: %d", (int)result);
            if(logFunc) logFunc(debug_msg);
            OTCloseProvider(state->endpoint);
            state->endpoint = kOTInvalidEndpointRef;
            return kOTStateChangeErr;
        }
    } else if (err != noErr) {
        sprintf(debug_msg, "TCP connection failed: %d", (int)err);
        if(logFunc) logFunc(debug_msg);
        OTCloseProvider(state->endpoint);
        state->endpoint = kOTInvalidEndpointRef;
        return err;
    }

    if(logFunc) logFunc("TCP connection established, starting SSL handshake...");

    /* Set the hostname for SNI (Server Name Indication) */
    ret = mbedtls_ssl_set_hostname(&state->ssl, hostname);
    if (ret != 0) {
        /* Non-fatal error, continue anyway but log it */
        sprintf(debug_msg, "Warning: SNI hostname setup failed: %d", ret);
        if(logFunc) logFunc(debug_msg);
    }

    /* Set up mbedTLS I/O functions to use Open Transport */
    mbedtls_ssl_set_bio(&state->ssl, state->endpoint, ot_send, ot_recv, NULL);

    if(logFunc) logFunc("Beginning SSL handshake (this may take a moment)...");

    /* Perform the SSL handshake with better error reporting */
    handshake_attempts = 0;

    while (1) {
        ret = mbedtls_ssl_handshake(&state->ssl);

        /* Add delay after every handshake attempt like original CodeWarrior version */
        Delay(5, NULL);  /* Wait 5/60th second for I/O operations */

        if (ret == 0) {
            /* Success! Get and show cipher */
            cipher = mbedtls_ssl_get_ciphersuite(&state->ssl);
            if(cipher != NULL) {
                sprintf(debug_msg, "SSL handshake successful! Connected with %s", cipher);
                if(logFunc) logFunc(debug_msg);
                ver = mbedtls_ssl_get_version(&state->ssl);
                sprintf(debug_msg, "Connected using TLS version: %s", ver ? ver : "unknown");
                if(logFunc) logFunc(debug_msg);
                return noErr;  /* Return immediately on success */
            } else {
                /* Cipher is NULL despite successful return -- shouldn't happen */
                if(logFunc) logFunc("Handshake appears successful but no cipher negotiated");
                return -1;  /* Return error if no cipher */
            }
        }

        /* Check for retriable errors */
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            /* These are normal during handshake - wait for I/O then try again */
            handshake_attempts++;

            /* Show progress periodically */
            if ((handshake_attempts % 5 == 0)) {
                sprintf(msg, "SSL handshake in progress... (attempt %d)", handshake_attempts);
                if(logFunc) logFunc(msg);
            }

            /* Prevent infinite loops */
            if (handshake_attempts > 100) {
                if (logFunc) logFunc("SSL handshake timeout -- too many attempts");
                return -1;
            }

            continue;
        }

        /* Other non-retriable error */
        if(logFunc) {
            sprintf(msg, "SSL handshake failed with error code: %d (0x%08x)", ret, (unsigned int)ret);
            logFunc(msg);

            /* Provide more specific error information */
            if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
                uint32_t verify_flags = mbedtls_ssl_get_verify_result(&state->ssl);
                sprintf(msg, "Certificate verification failed with flags: 0x%08lx", (unsigned long)verify_flags);
                logFunc(msg);

                if (verify_flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
                    logFunc("  - Certificate not trusted by CA");
                }
                if (verify_flags & MBEDTLS_X509_BADCERT_CN_MISMATCH) {
                    logFunc("  - Hostname mismatch");
                }
                if (verify_flags & MBEDTLS_X509_BADCERT_EXPIRED) {
                    logFunc("  - Certificate expired");
                }
            }
        }

        /* Return the error code */
        return ret;
    }
}

/* Send data over SSL connection */
OSStatus SSL_Send(SSLState* state, const void* buffer, size_t length, size_t* bytesSent, LoggingCallback logFunc)
{
    int ret;
    size_t total_sent = 0;
    int retries = 0;
    const int max_retries = 10;
    char msg[100];

    if (!state->initialized || state->endpoint == kOTInvalidEndpointRef) {
        if (logFunc) logFunc("Error: SSL not initialized or no endpoint");
        *bytesSent = 0;
        return paramErr;
    }

    if (logFunc) logFunc("Attempting to send data...");

    /* Keep trying until all data is sent or error */
    while (total_sent < length && retries < max_retries) {
        ret = mbedtls_ssl_write(&state->ssl,
                               (const unsigned char*)buffer + total_sent,
                               length - total_sent);

        if (ret > 0) {
            /* Data sent successfully */
            total_sent += ret;

            sprintf(msg, "Sent %d bytes (total: %lu of %lu)", ret, (unsigned long)total_sent, (unsigned long)length);
            if (logFunc) logFunc(msg);
        }
        else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ) {
            /* Need to wait - retry */
            retries++;

            sprintf(msg, "SSL write would block (retry %d of %d)", retries, max_retries);
            if (logFunc) logFunc(msg);

            continue;
        }
        else {
            /* Error occurred */
            sprintf(msg, "SSL write error: %d (0x%08x)", ret, (unsigned int)ret);
            if (logFunc) logFunc(msg);

            *bytesSent = total_sent;
            return ret;
        }
    }

    if (total_sent < length) {
        if (logFunc) logFunc("Failed to send all data after maximum retries");
        *bytesSent = total_sent;
        return -1;
    }

    *bytesSent = total_sent;
    if (logFunc) logFunc("Data sent successfully");
    return noErr;
}

/* Receive data from SSL connection */
OSStatus SSL_Receive(SSLState* state, void* buffer, size_t bufferSize, size_t* bytesReceived, LoggingCallback logFunc)
{
    int ret;
    char msgBuf[100];
    int max_retries = 30;
    int retry_count = 0;
    OTResult endpoint_state;

    if (!state->initialized || state->endpoint == kOTInvalidEndpointRef) {
        if (logFunc) logFunc("Error: SSL not initialized or no endpoint");
        return paramErr;
    }

    /* Check endpoint state before attempting to receive */
    endpoint_state = OTGetEndpointState(state->endpoint);
    sprintf(msgBuf, "Endpoint state: %d", (int)endpoint_state);
    if (logFunc) logFunc(msgBuf);

    /* Only proceed if endpoint is in data transfer state */
    if (endpoint_state != T_DATAXFER) {
        if (logFunc) logFunc("Error: Endpoint not in data transfer state");
        *bytesReceived = 0;
        return -1;
    }

    /* Try to read data with retries */
    if (logFunc) logFunc("Attempting to read data from server...");

    while (retry_count < max_retries) {
        ret = mbedtls_ssl_read(&state->ssl, buffer, bufferSize);

        if (ret > 0) {
            /* Data received successfully */
            *bytesReceived = ret;
            sprintf(msgBuf, "Successfully received %d bytes", ret);
            if (logFunc) logFunc(msgBuf);
            return noErr;
        }
        else if (ret == 0) {
            /* Connection closed by server */
            *bytesReceived = 0;
            if (logFunc) logFunc("Connection closed by server (clean shutdown)");
            return noErr;
        }
        else if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            /* No data available yet - retry after a short delay */
            retry_count++;

            if (retry_count % 5 == 0 && logFunc) {
                sprintf(msgBuf, "Waiting for data (retry %d of %d)...", retry_count, max_retries);
                logFunc(msgBuf);
            }

            /* Give the server some time to send data */
            Delay(1, NULL);  /* 1/60th of a second */
            continue;
        }
        else if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
            /* Read timeout */
            *bytesReceived = 0;
            if (logFunc) logFunc("Read timeout occurred - server took too long to respond");
            return -1;
        }
        else {
            /* Other error occurred */
            *bytesReceived = 0;
            sprintf(msgBuf, "SSL read error: %d (0x%08x)", ret, (unsigned int)ret);
            if (logFunc) logFunc(msgBuf);
            return ret;
        }
    }

    /* If we get here, we've exhausted our retries */
    if (logFunc) logFunc("Timeout waiting for data from server");
    *bytesReceived = 0;
    return -1;
}

/* Close the SSL connection and clean up */
void SSL_Close(SSLState* state)
{
    if (state == NULL || !state->initialized)
        return;

    mbedtls_ssl_close_notify(&state->ssl);

    /* Close the endpoint */
    if (state->endpoint != kOTInvalidEndpointRef) {
        /* Close the endpoint */
        OTCloseProvider(state->endpoint);
        state->endpoint = kOTInvalidEndpointRef;
    }

    /* Free the large buffer */
    if(state->large_buffer != NULL) {
        mac_free(state->large_buffer);
        state->large_buffer = NULL;
    }

    /* Free SSL resources */
    mbedtls_ssl_free(&state->ssl);
    mbedtls_ssl_config_free(&state->conf);
    mbedtls_ctr_drbg_free(&state->ctr_drbg);
    mbedtls_entropy_free(&state->entropy);
    mbedtls_x509_crt_free(&state->cacert);
    mbedtls_x509_crt_free(&state->clicert);
    mbedtls_pk_free(&state->pkey);

    memset(state, 0, sizeof(SSLState));
    state->initialized = 0;
}

/* Load the root CA certificate */
int load_root_ca_cert(SSLState* state, const char* ca_cert_pem, size_t ca_cert_len, LoggingCallback logFunc)
{
    int ret;
    char error_buf[100];
    char msg[150];

    if(logFunc) {
        sprintf(msg, "Certificate length: %lu bytes", (unsigned long)ca_cert_len);
        logFunc(msg);
    }

    /* Parse the certificate (include null terminator) */
    ret = mbedtls_x509_crt_parse(&state->cacert,
                                (const unsigned char *)ca_cert_pem,
                                ca_cert_len + 1);

    if(ret < 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        if(logFunc) {
            sprintf(msg, "Failed to parse CA certificate: %s (code %d)", error_buf, ret);
            logFunc(msg);
        }
        return ret;
    }

    /* Configure certificate */
    if(logFunc) logFunc("CA certificate parsed successfully");
    mbedtls_ssl_conf_ca_chain(&state->conf, &state->cacert, NULL);
    return 0;
}