/* 
 * SSLWrapper.c
 * 
 * SSL wrapper implementation for Classic Mac OS using PolarSSL/mbedTLS
 */


#include "Globals.h"
#include "SSLWrapper.h"
#include <string.h>
#include <stdio.h>
#include <LowMem.h>
#include <Resources.h>
#include <Processes.h>
#include <Folders.h>
#include <Files.h>
#include <Gestalt.h>
#include "api.h"
#include "debug.h"
#include "ssl_ciphersuites.h"
#include "memory_buffer_alloc.h"
#include "platform.h"
#include "Logging.h"


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

#ifdef __powerc
asm unsigned long GetPPC601Timer(void)
{
	mfspr r3, 5
	blr
}

asm unsigned long GetPPC603PlusTimer(void)
{
	mftb r3
	blr
}
#endif

/* Get the PowerPC timebase register - returns lower 32 bits */
static unsigned long GetPPCTimer(bool is601)
{
#ifdef __powerc
	if(is601)
		return GetPPC601Timer();
	else
		return GetPPC603PlusTimer();
#else
    /* For 68K, use Microseconds() */
    UnsignedWide usec;
    Microseconds(&usec);
    return usec.lo;
#endif
}

/* Get time base resolution - ticks per millisecond */
static double GetTimeBaseResolution(void)
{
    long speed;
    
    /* Try to get processor clock speed */
    if (Gestalt(gestaltProcClkSpeed, &speed) != noErr)
        return 6000.0; /* Default for 60MHz PowerPC */
        
    /* Assume 10 cycles per timebase tick as per PowerPC spec */
    return speed / 1.0e4;
}

/* Fill buffer with enhanced entropy from Mac-specific sources */
int mac_entropy_gather(unsigned char *output, size_t len)
{
    size_t pos = 0;
    static UInt32 entropy_counter = 0;
    double timebase_ticks_per_msec;
    bool is_powerpc = false;
    bool is_601 = false;
    long cpu_type;
    unsigned long disk_data[8];
    ProcessSerialNumber psn;
    ProcessInfoRec process_info;
    Point mouse_point;
    size_t i;
    
    /* Determine CPU type for timebase access method */
    if (Gestalt(gestaltNativeCPUtype, &cpu_type) == noErr) {
        is_powerpc = (cpu_type >= gestaltCPU601);
        is_601 = (cpu_type == gestaltCPU601);
    }
    
    /* Get timebase resolution */
    timebase_ticks_per_msec = GetTimeBaseResolution();
    
    /* Increment our own counter each time we're called */
    entropy_counter++;

    /* Gather initial entropy sources in small discrete chunks */
    while (pos < len) {
        UInt32 block[4];
        size_t block_size = sizeof(block);
        
        /* Only use as much as we need for the final block */
        if (pos + block_size > len)
            block_size = len - pos;
            
        /* 1. System timer with high precision */
        block[0] = GetPPCTimer(is_601);
        
        /* 2. Various time sources that have different resolutions */
        block[1] = TickCount();
        GetDateTime(&block[2]);
        
        /* 3. Mouse location and low-memory globals */
        mouse_point = LMGetMouseLocation();
        block[3] = (UInt32)((mouse_point.h << 16) | (mouse_point.v & 0xFFFF));;
        block[3] ^= (UInt32)LMGetCurApName();
        block[3] ^= (UInt32)LMGetScrnBase();
        
        /* 4. Mix in the entropy counter */
        for (i = 0; i < 4; i++) {
            block[i] ^= entropy_counter * (0x1f1f1f1f + i);
        }
        
        /* Copy this block to the output */
        memcpy(output + pos, block, block_size);
        pos += block_size;
        
        /* Change entropy_counter to ensure different blocks */
        entropy_counter = (entropy_counter * 1103515245 + 12345) & 0x7fffffff;
    }
    
    /* Now, do a second pass to gather even more system state 
       and mix it with what we already have */
       
    /* Get startup volume information */
    {
        short vRefNum;
        long dirID;
        XVolumeParam pb;
        OSErr err;
        
        /* Try to get information about the startup disk */
        FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder,
                  &vRefNum, &dirID);
        pb.ioVRefNum = vRefNum;
        pb.ioCompletion = 0;
        pb.ioNamePtr = 0;
        pb.ioVolIndex = 0;
        //err = PBXGetVolInfoSync(&pb);
        
        /*
        WE'RE GOING TO HAVE TO FIX THIS
        THIS SHOULD BE VOLUME INFO NOT TICKCOUNT AND RANDOM
        */
        
        if (err == noErr) {
            /* Use volume information for entropy */
            disk_data[0] = TickCount();
            disk_data[1] = Random();
            disk_data[2] = LMGetTicks();
            disk_data[3] = (UInt32)CurResFile();
        }
    }
    
    /* Get information about the current process */
    {
        process_info.processInfoLength = sizeof(ProcessInfoRec);
        process_info.processName = nil;
        process_info.processAppSpec = nil;
        
        GetCurrentProcess(&psn);
        GetProcessInformation(&psn, &process_info);
        
        disk_data[4] = process_info.processSize;
        disk_data[5] = process_info.processMode;
        disk_data[6] = (UInt32)(long)process_info.processLocation;
        disk_data[7] = process_info.processFreeMem;
    }
    
    /* Mix the additional entropy into our output buffer */
    for (i = 0; i < len; i++) {
        output[i] ^= ((unsigned char*)disk_data)[i % sizeof(disk_data)];
    }
    
    /* Add a final non-linear transformation */
    for (i = 0; i < len; i++) {
        /* Apply an S-box like transformation */
        unsigned char b = output[i];
        b = ((b << 1) | (b >> 7)) ^ ((b << 2) | (b >> 6));
        output[i] = b ^ (entropy_counter & 0xFF);
        entropy_counter = (entropy_counter * 214013 + 2531011) & 0x7fffffff;
    }

    return 0; /* Success */
}

/* Custom entropy function for mbedTLS */
int mac_entropy_func(void *data, unsigned char *output, size_t len)
{
    /* Call our enhanced entropy gathering function */
    return mac_entropy_gather(output, len);
}

/* 
 * Custom send/receive functions for PolarSSL that use Open Transport
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
OSStatus SSL_Initialize(SSLState* state, LoggingCallback logFunc) {
    int ret;
    UInt32 ticks;
    char seed_buf[64];
    char errorMsg[100];
    unsigned char enhanced_seed[128];
    UInt32 fallback_seed[8];
    Point mouse_point;
    char suite_name[100];
    int j;
    char suite_msg[100];
    char curve_msg[100];
    char debug_msg[100];

	static unsigned char dhm_P[]={
		0x0F,0x52,0xE5,0x24,0xF5,0xFA,0x9D,0xDC,0xC6,0xAB,0xE6,0x04,
		0xE4,0x20,0x89,0x8A,0xB4,0xBF,0x27,0xB5,0x4A,0x95,0x57,0xA1,
		0x06,0xE7,0x30,0x73,0x83,0x5E,0xC9,0x23,0x11,0xED,0x42,0x45,
		0xAC,0x49,0xD3,0xE3,0xF3,0x34,0x73,0xC5,0x7D,0x00,0x3C,0x86,
		0x63,0x74,0xE0,0x75,0x97,0x84,0x1D,0x0B,0x11,0xDA,0x04,0xD0,
		0xFE,0x4F,0xB0,0x37,0xDF,0x57,0x22,0x2E,0x96,0x42,0xE0,0x7C,
		0xD7,0x5E,0x46,0x29,0xAF,0xB1,0xF4,0x81,0xAF,0xFC,0x9A,0xEF,
		0xFA,0x89,0x9E,0x0A,0xFB,0x16,0xE3,0x8F,0x01,0xA2,0xC8,0xDD,
		0xB4,0x47,0x12,0xF8,0x29,0x09,0x13,0x6E,0x9D,0xA8,0xF9,0x5D,
		0x08,0x00,0x3A,0x8C,0xA7,0xFF,0x6C,0xCF,0xE3,0x7C,0x3B,0x6B,
		0xB4,0x26,0xCC,0xDA,0x89,0x93,0x01,0x73,0xA8,0x55,0x3E,0x5B,
		0x77,0x25,0x8F,0x27,0xA3,0xF1,0xBF,0x7A,0x73,0x1F,0x85,0x96,
		0x0C,0x45,0x14,0xC1,0x06,0xB7,0x1C,0x75,0xAA,0x10,0xBC,0x86,
		0x98,0x75,0x44,0x70,0xD1,0x0F,0x20,0xF4,0xAC,0x4C,0xB3,0x88,
		0x16,0x1C,0x7E,0xA3,0x27,0xE4,0xAD,0xE1,0xA1,0x85,0x4F,0x1A,
		0x22,0x0D,0x05,0x42,0x73,0x69,0x45,0xC9,0x2F,0xF7,0xC2,0x48,
		0xE3,0xCE,0x9D,0x74,0x58,0x53,0xE7,0xA7,0x82,0x18,0xD9,0x3D,
		0xAF,0xAB,0x40,0x9F,0xAA,0x4C,0x78,0x0A,0xC3,0x24,0x2D,0xDB,
		0x12,0xA9,0x54,0xE5,0x47,0x87,0xAC,0x52,0xFE,0xE8,0x3D,0x0B,
		0x56,0xED,0x9C,0x9F,0xFF,0x39,0xE5,0xE5,0xBF,0x62,0x32,0x42,
		0x08,0xAE,0x6A,0xED,0x88,0x0E,0xB3,0x1A,0x4C,0xD3,0x08,0xE4,
		0xC4,0xAA,0x2C,0xCC,0xB1,0x37,0xA5,0xC1,0xA9,0x64,0x7E,0xEB,
		0xF9,0xD3,0xF5,0x15,0x28,0xFE,0x2E,0xE2,0x7F,0xFE,0xD9,0xB9,
		0x38,0x42,0x57,0x03,
		};
		
	static const unsigned char dhm_G[] = {0x02};

    

    
    /* Supported elliptic curves in order of preference */    
    static mbedtls_ecp_group_id curve_list[] = {
    	MBEDTLS_ECP_DP_SECP256R1,
    	MBEDTLS_ECP_DP_NONE
    };
    
    /*
    const mbedtls_ecp_group_id curve_list[] = {
        //MBEDTLS_ECP_DP_CURVE25519,  /* Most modern servers prefer this */
        //MBEDTLS_ECP_DP_SECP256R1,    /* Common fallback */
        //MBEDTLS_ECP_DP_SECP384R1,    /* Stronger NIST curve */
        //MBEDTLS_ECP_DP_SECP521R1,    /* Strongest NIST curve */
      //  MBEDTLS_ECP_DP_NONE
    //};
    
    /* Set cipher suites that are most likely to be compatible */
    static const int ciphersuites[] = {
    	// MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,	/* Prioritized */
        //MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,			/* 0xc02f */
        //MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,     		/* Most compatible */
        //MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,     		/* Strong and compatible */
        MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,           		/* Fallback if ECDHE fails */
        MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA,           		/* Stronger fallback */
        0  /* Terminator */
    };
    
    /* Initialize the log file */
    OSErr fileErr = InitializeLogFile();
    if(fileErr != noErr)
    {
    	if(logFunc) {
    		char msg[100];
    		sprintf(msg, "Error initializing log file %d", fileErr);
    		logFunc(msg);
    	}
    }
    
    DualLogFunc("Starting SSL initialization");
    
    if (state == NULL) {
        if (logFunc) logFunc("Error: NULL state pointer");
        return paramErr;
    }
    
    /* If already initialized, clean up first */
    if (state->initialized) {
        if (logFunc) logFunc("Found existing SSL state, cleaning up...");
        SSL_Close(state);
    }
    
    /* Log start of initialization */
    if (logFunc) logFunc("Starting SSL initialization");
    
    /* Clear the state structure */
    memset(state, 0, sizeof(SSLState));
    
    /* Allocate the large buffer for handshake operations */
    state->buffer_size = 16384; //16k should be enough
    state->large_buffer = (unsigned char*)mbedtls_calloc(1, state->buffer_size);
    if(state->large_buffer == NULL){
    	if(logFunc) logFunc("Error: Failed to allocate large buffer");
    	return memFullErr;
    }
    
    /* add the custom calloc and free */
    mbedtls_platform_set_calloc_free(mbedtls_calloc, mbedtls_free);
    
    /* Set up the heap buffer */
    mbedtls_memory_buffer_alloc_init(heap_buf, sizeof(heap_buf));
    
    /* STEP 1: Gather entropy */
    if (logFunc) logFunc("Step 1: Gathering entropy sources");
    
    /* Create a better entropy source using Classic Mac resources */
    ticks = TickCount(); /* Get current tick count from Mac OS */
    
    /* Format info message */
    sprintf(errorMsg, "  System ticks: %lu", (unsigned long)ticks);
    if (logFunc) logFunc(errorMsg);
    
    /* Create a seed buffer with various system data */
    memset(seed_buf, 0, sizeof(seed_buf));
    BlockMoveData(&ticks, seed_buf, sizeof(ticks));
    
    /* Use system time for additional entropy */
    {
        unsigned long dateTime;
        GetDateTime(&dateTime);
        
        /* Format info message */
        sprintf(errorMsg, "  System date/time: %lu", dateTime);
        if (logFunc) logFunc(errorMsg);
        
        BlockMoveData(&dateTime, &seed_buf[4], sizeof(dateTime));
    }
    
    /* Add some additional entropy sources if available */
    if (LMGetCurApName() != NULL) {
        /* Format info message */
        sprintf(errorMsg, "  App name first byte: 0x%02X", (unsigned char)LMGetCurApName()[0]);
        if (logFunc) logFunc(errorMsg);
        
        seed_buf[12] = LMGetCurApName()[0];
    }
    
    /* Fill rest with pseudo-random values */
    {
        int i;
        for (i = 16; i < sizeof(seed_buf); i++) {
            seed_buf[i] = (char)((ticks * i * 0xC13F) & 0xFF);
        }
    }
    if (logFunc) logFunc("  Entropy gathering completed");
    
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
    mac_entropy_gather(enhanced_seed, sizeof(enhanced_seed));
    
    /* Log the first few bytes for debugging */
    if (logFunc) {
        char seed_debug[100];
        sprintf(seed_debug, "  Seed preview: %02X %02X %02X %02X %02X %02X %02X %02X",
            enhanced_seed[0], enhanced_seed[1], enhanced_seed[2], enhanced_seed[3],
            enhanced_seed[4], enhanced_seed[5], enhanced_seed[6], enhanced_seed[7]);
        logFunc(seed_debug);
    }
    
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
    
    
    /* Configure specific parameters for key derivation */
    mbedtls_ssl_conf_legacy_renegotiation(&state->conf, MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION);
    
    state->conf.endpoint = MBEDTLS_SSL_IS_CLIENT;
    state->conf.transport = MBEDTLS_SSL_TRANSPORT_STREAM;
    
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
    
    /* Set debug threshold (0-5, higher = more verbose) */
    mbedtls_debug_set_threshold(5);
    
    /* Set up debug callback */
    mbedtls_ssl_conf_dbg(&state->conf, ssl_debug_callback, logFunc);
    
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
    
    /* Log the ciphersuites loaded */
    if(logFunc) {
    	int *current_ciphers = mbedtls_ssl_list_ciphersuites();
    	int i = 0;
    	
    	logFunc("Configured cipher suites: ");
    	while(current_ciphers[i] != 0){
    		sprintf(debug_msg, "Suite %d: 0x%04X", i, current_ciphers[i]);
    		logFunc(debug_msg);
			i++;
    	}
    }
    
    /* Enable PKCS#1 v1.5  padding */
    mbedtls_rsa_set_padding(NULL, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_SHA256);
    
    mbedtls_ssl_conf_dhm_min_bitlen(&state->conf, 1024);
    
    /* Enable renegotiation */ 
    mbedtls_ssl_conf_renegotiation(&state->conf, MBEDTLS_SSL_RENEGOTIATION_ENABLED);
    
    if (logFunc) {
        logFunc("  Cipher suites being configured");
        for(j = 0 ; ciphersuites[j] != 0 ; j++) {
            sprintf(suite_msg, "  Suite %d: 0x%04X (%s)", j, ciphersuites[j],
                    mbedtls_ssl_get_ciphersuite_name(ciphersuites[j]));
            logFunc(suite_msg);
        }
        logFunc(" Ciphersuites set to compataible options");
    }
    
    /* Set RNG function */
    mbedtls_ssl_conf_rng(&state->conf, mbedtls_ctr_drbg_random, &state->ctr_drbg);
    if (logFunc) logFunc("  RNG function configured");
    
    /* Load the root CA certificate */
    ret = load_root_ca_cert(state, ca_cert_pem, strlen(ca_cert_pem), logFunc);
    if(ret != 0) {
        if(logFunc) logFunc("Warning: Failed to load CA certificates");
    }
    
    /* Set certificate verification callback */
    mbedtls_ssl_conf_verify(&state->conf, ssl_verify_callback, logFunc);
    
    /* Use MBEDTLS_SSL_VERIFY_REQUIRED for strictly enforced verification */
    mbedtls_ssl_conf_authmode(&state->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    if (logFunc) logFunc("Certificate verification REQUIRED");
    
    /* Set TLS 1.2 for compatibility */
    mbedtls_ssl_conf_min_version(&state->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_2);
    mbedtls_ssl_conf_max_version(&state->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_2);
    if (logFunc) logFunc("  SSL/TLS version TLS 1.1");
    
    /* Configure signature algorithms */
    mbedtls_ssl_conf_sig_hashes(&state->conf, sig_algs);
    if(logFunc) logFunc("  Signature algorithms configured as sig_algs");
    
    /* Dumping the curve list for reasons */
    if(logFunc){
    	int i;
    	for(i = 0; curve_list[i] != MBEDTLS_ECP_DP_NONE; i++){
    		const mbedtls_ecp_curve_info *info = mbedtls_ecp_curve_info_from_grp_id(curve_list[i]);
    		if(info == NULL)
    		{
    			sprintf(curve_msg, "ERROR: Curve ID %d not found in internal curve info table", curve_list[i]);
    			logFunc(curve_msg);
    		} else {
    			sprintf(curve_msg, "Curve %s (ID %d) found", info->name, info->grp_id);
    			logFunc(curve_msg);
    		}
    	}
    }
    
    
    /* Configure elliptic curves */
    mbedtls_ssl_conf_curves(&state->conf, curve_list);
    
    /* Add debug log for elliptic curves */
    if(logFunc) {
        int i;
        logFunc("Supported elliptic curves:");
        for(i = 0; curve_list[i] != MBEDTLS_ECP_DP_NONE; i++) {
            const mbedtls_ecp_curve_info *curve_info = mbedtls_ecp_curve_info_from_grp_id(curve_list[i]);
            if (curve_info != NULL) {
                sprintf(curve_msg, "  Curve %d: %s (id: %d)", i, curve_info->name, curve_info->grp_id);
                logFunc(curve_msg);
            } else {
                sprintf(curve_msg, "  Curve %d: Unknown (id: %d)", i, curve_list[i]);
                logFunc(curve_msg);
            }
        }
        logFunc(" Elliptic curves configured");
    }
    
    if(logFunc) {
        int i;
        logFunc("Offering the following cipher suites:");
        for(i = 0; ciphersuites[i] != 0; i++) {
            sprintf(suite_name, " Suite %d: %s", i, mbedtls_ssl_get_ciphersuite_name(ciphersuites[i]));
            logFunc(suite_name);
        }
    }
    
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
OSStatus SSL_Connect(SSLState* state, InetAddress* address, TEHandle responseText, LoggingCallback logFunc)
{
    OSStatus err;
    OTResult result;
    TBind bindReq;
    TCall sndCall;
    int ret;
    int handshake_attempts;
    char debug_msg[150];
    unsigned char alert_level;
	unsigned char alert_type;
	char msg[100];
	char errMsg[100];
	UInt32 delayStart;
	char* alert_desc = "Unknown";
	const char* cipher;
	const char* ver;
	const mbedtls_x509_crt *peer_cert;
	char cert_info[1024];
    const int *hashes;
    char hash_msg[100];
    
    char* debugBuffer = NewPtr(65536);
    long debugBufferUsed = 0;
    
    
    if (!state->initialized)
        return paramErr;
        
    DualLogFunc("Setting Up SSL Connection...");
    
    
    
    /* Create the TCP endpoint */
    err = CreateSecureEndpoint(&state->endpoint);
    if (err != noErr) {
        if(responseText != NULL) {
            sprintf(debug_msg, "Failed to create secure endpoint: %d", (int)err);
            if(logFunc) logFunc(debug_msg);
        }
        return err;
    }
        
    /* Debug output */
    if(responseText != NULL)
    {
        if(logFunc) logFunc("Setting up SSL connection...");
    }
    
    /* Bind the endpoint */
    bindReq.addr.maxlen = 0;
    bindReq.addr.len = 0;
    bindReq.addr.buf = NULL;
    bindReq.qlen = 0;
    
    err = OTBind(state->endpoint, &bindReq, NULL);
    if (err != noErr) {
        if(responseText != NULL) {
            sprintf(debug_msg, "Failed to bind endpoint: %d", (int)err);
            if(logFunc) logFunc(debug_msg);
        }
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
    if(responseText != NULL)
    {
        if(logFunc) logFunc("Establishing TCP connection...");
    }
    
    err = OTConnect(state->endpoint, &sndCall, NULL);
    
    /* Check for asynchronous completion */
    if (err == kOTNoDataErr) {
        if(responseText != NULL)
        {
            if(logFunc) logFunc("Waiting for connection completion...");
        }
        
        result = OTLook(state->endpoint);
        
        if (result == T_CONNECT) {
            /* Accept the connection and finish connecting */
            err = OTRcvConnect(state->endpoint, NULL);
            if (err != noErr) {
                if(responseText != NULL) {
                    sprintf(debug_msg, "TCP connection failed during completion: %d", (int)err);
                    if(logFunc) logFunc(debug_msg);
                }
                OTCloseProvider(state->endpoint);
                state->endpoint = kOTInvalidEndpointRef;
                return err;
            }
        } else {
            if(responseText != NULL) {
                sprintf(debug_msg, "Unexpected connection state: %d", (int)result);
                if(logFunc) logFunc(debug_msg);
            }
            OTCloseProvider(state->endpoint);
            state->endpoint = kOTInvalidEndpointRef;
            return kOTStateChangeErr;
        }
    } else if (err != noErr) {
        if(responseText != NULL) {
            sprintf(debug_msg, "TCP connection failed: %d", (int)err);
            if(logFunc) logFunc(debug_msg);
        }
        OTCloseProvider(state->endpoint);
        state->endpoint = kOTInvalidEndpointRef;
        return err;
    }
    
    /* Debug output */
    if(responseText != NULL)
    {
        if(logFunc) logFunc("TCP connection established, starting SSL handshake...");
    }
    
    /* Set the hostname for SNI (Server Name Indication) */
    ret = mbedtls_ssl_set_hostname(&state->ssl, API_HOST);
    if (ret != 0) {
        /* Non-fatal error, continue anyway but log it */
        if(responseText != NULL)
        {
            sprintf(debug_msg, "Warning: SNI hostname setup failed: %d", ret);
            if(logFunc) logFunc(debug_msg);
        }
    }
    
    /* Log hostname */
    if(logFunc)
    {
    	sprintf(debug_msg, "Using hostname for SNI: %s", API_HOST);
    	logFunc(debug_msg);
    }
    
    /* Set up mbedTLS I/O functions to use Open Transport */
    mbedtls_ssl_set_bio(&state->ssl, state->endpoint, ot_send, ot_recv, NULL);
    
    /* Debug output */
    if(responseText != NULL)
    {
        if(logFunc) logFunc("Beginning SSL handshake (this may take a moment)...");
    }
    


    mbedtls_ssl_conf_dbg(&state->conf, ssl_debug_callback, logFunc);
    
    if(logFunc){
    	logFunc("Debugging signature algorithms extension: ");
    	
    	//get access to the sig_hashes array
    	hashes = state->conf.sig_hashes;
    	
    	//print each hash algorithm
    	if(hashes != NULL)
    	{
    		int i;
    		i = 0;
    		while(hashes[i] != 0)
    		{

    			sprintf(hash_msg, " Hash algorithm %d: 0x%04X", i, hashes[i]);
    			logFunc(hash_msg);
    			i++;
    		}
    	} else {
    		logFunc("No signature hashes configured!");
    	}
    }
    
    //mbedtls_debug_set_threshold(5);
    
    /* Perform the SSL handshake with better error reporting */
    handshake_attempts = 0;
    

	while (1) {
    	ret = mbedtls_ssl_handshake(&state->ssl);
    	
    	
    	
    	Delay(5, NULL);  //Delay for 5/60th of a second for reasons
      
    	if (ret == 0) {
        	/* Success! Get and show cipher */
        	cipher = mbedtls_ssl_get_ciphersuite(&state->ssl);
        	if(cipher != NULL) {
            	sprintf(debug_msg, "SSL handshake successful! Connected with %s", cipher);
            	if(logFunc) logFunc(debug_msg);
            	ver = mbedtls_ssl_get_version(&state->ssl);
            	sprintf(debug_msg, "Connected using TLS version: %s", ver ? ver : "unknown");
            	if(logFunc) logFunc(debug_msg);
            	
            	peer_cert = mbedtls_ssl_get_peer_cert(&state->ssl);
            	if(peer_cert != NULL){
            		const mbedtls_x509_crt *cert_iter = peer_cert;
            		int cert_num = 0;
            		
            		if(logFunc) logFunc("Peer certificate chain received:");
            		
            		while(cert_iter != NULL)
            		{
            			sprintf(debug_msg, " Certificate #%d:", cert_num);
            			if(logFunc) logFunc(debug_msg);
            			
            			mbedtls_x509_crt_info(cert_info, sizeof(cert_info), "   ",cert_iter);
            			if(logFunc) logFunc(cert_info);
            			
            			cert_iter = cert_iter->next;
            			cert_num++;
            		}
            	} else {
            		if(logFunc) logFunc("No peer certificate was received");
            	}
            	return noErr;  /* Return immediately on success */
        	} else {
            	/* Cipher is NULL despite successful return -- shouldn't happen */
            	if(logFunc) logFunc("Handshake appears successful but no cipher negotiated");
            	return -1;  /* Return error if no cipher */
        	}
    	}
    
    	/* Log the error */
    	sprintf(errMsg, "Handshake step returned: %d (0x%x)", ret, ret);
    	if(logFunc) logFunc(errMsg);
    	
    
    	/* Check for retriable errors */
    	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        	/* These are normal during handshake - try again */
        	handshake_attempts++;
        
        	/* Show progress periodically */
        	if (responseText != NULL && (handshake_attempts % 5 == 0)) {
            	sprintf(msg, "SSL handshake in progress... (attempt %d)", handshake_attempts);
            	logFunc(msg);
        	}
        
        	/* Prevent infinite loops */
        	if (handshake_attempts > 100) {
            	if (logFunc) logFunc("SSL handshake timeout -- too many attempts");
            	return -1;
        	}
        
        	continue;
        	Delay(1, NULL);
    	}
    	
    	/* Certificate Verification Failure Detail */
    	if(ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
    		uint32_t flags = mbedtls_ssl_get_verify_result(&state->ssl);
    		char verify_buf[1024];
    		mbedtls_x509_crt_verify_info(verify_buf, sizeof(verify_buf), " ! ", flags);
    		if(logFunc) {
    			logFunc("Certificat verification details:");
    			logFunc(verify_buf);
    		}
    	}  
    
    	/* Handle fatal alerts */
    	if(ret == MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE) {
    		peer_cert = mbedtls_ssl_get_peer_cert(&state->ssl);
    		if(peer_cert != NULL)
    		{
    			if(logFunc) logFunc("Got peer certificate but handshake still failed");
    			
    			mbedtls_x509_crt_info(cert_info, sizeof(cert_info), " ", peer_cert);
    			logFunc(cert_info);
    		} else {
    			if(logFunc) logFunc("No peer certificate received");
    		}
    		
        	if(logFunc) {
           	 sprintf(msg, "Server sent fatal alert during handshake code %d", ret);
            	logFunc(msg);
            
            	/* Try to get more info about the alert */
            	alert_level = state->ssl.in_msg[0];
            	alert_type = state->ssl.in_msg[1];
            
            	switch(alert_type) {
                	case 40: alert_desc = "handshake failure"; break;
                	case 42: alert_desc = "bad certificate"; break;
                	case 43: alert_desc = "unsupported cert"; break;
                	case 47: alert_desc = "illegal param"; break;
                	case 51: alert_desc = "no renegotation"; break;
                	case 70: alert_desc = "protocol version"; break;
                	case 71: alert_desc = "insufficient security"; break;
      	      }
            
    	        sprintf(msg, "Alert level: %d, type %d (%s)", alert_level, alert_type, alert_desc);
    	        logFunc(msg);
     	   }
    	}
    
    	/* Other non-retriable error */
    	if(logFunc) {
    	    sprintf(msg, "SSL handshake failed with error code: %d", ret);
    	    logFunc(msg);
    	}
    
    	/* Return the error code */
    	return ret;
	}
}

/* Enhance the SSL_Send function */
OSStatus SSL_Send(SSLState* state, const void* buffer, size_t length, size_t* bytesSent, LoggingCallback logFunc)
{
    int ret;
    size_t total_sent = 0;
    int retries = 0;
    const int max_retries = 10;
    char msg[100];
    char error_buf[100];
    
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
            
            /* Add specific error code checks */
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            	if (logFunc) logFunc(" ERROR: connection closed by peer");
            else if(ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            	if (logFunc) logFunc(" ERROR: Write operation would block");
            else if(ret == -0x6880)
            	if (logFunc) logFunc(" ERROR: Read operation would block");
            else if(ret == MBEDTLS_ERR_SSL_TIMEOUT)
            	if (logFunc) logFunc(" ERROR: Timeout");
            else if(ret == MBEDTLS_ERR_SSL_BAD_INPUT_DATA)
            	if (logFunc) logFunc(" Error: Bad input parameters");
            
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

/* Receive data with internal retry loop and endpoint state checking */
OSStatus SSL_Receive(SSLState* state, void* buffer, size_t bufferSize, size_t* bytesReceived, LoggingCallback logFunc)
{
    int ret;
    char msgBuf[100];
    int max_retries = 30;  /* Adjust based on your needs */
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
    
    /* Only proceed if endpoint is in data transfer state (T_DATAXFER is typically 5) */
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
                
                /* Check endpoint state again during retries */
                endpoint_state = OTGetEndpointState(state->endpoint);
                sprintf(msgBuf, "Endpoint state during retry: %d", (int)endpoint_state);
                logFunc(msgBuf);
                
                if (endpoint_state != T_DATAXFER) {
                    if (logFunc) logFunc("Error: Endpoint no longer in data transfer state");
                    *bytesReceived = 0;
                    return -1;
                }
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
    if(state->large_buffer != NULL){
    	mbedtls_free(state->large_buffer);
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

/* Callback function to route debug messages to logFunc */
static void ssl_debug_callback(void *ctx, int level, const char *file, int line, const char *str)
{	
	char main_debug_msg[200];

	LoggingCallback logFunc = (LoggingCallback)ctx;
	
	sprintf(main_debug_msg, "SSL DBG [%d] %s", level, str);
	
	DirectLogMessage(main_debug_msg);
	
	if(logFunc)
	{
		logFunc(main_debug_msg);
	}
}

/* Load the root CA */
int load_root_ca_cert(SSLState* state, const char* ca_cert_pem, size_t ca_cert_len, LoggingCallback logFunc)
{
	int ret;
	char error_buf[100];
	char debug_info[100];
	int i;
	int j;
	char msg[150];
	int dump_len;
	char hex_row[100];
	char hex_byte[4];

	sprintf(debug_info, "Certificat length: %lu bytes", (unsigned long)ca_cert_len);
	if(logFunc) logFunc(debug_info);
		
	/* dump first 256 bytes of cert */
	dump_len = (ca_cert_len < 256) ? ca_cert_len : 256;
	sprintf(msg, "First %d bytes of certificate:", dump_len);
	if(logFunc) logFunc(msg);
		
	for(i = 0; i < dump_len; i += 16)
	{
		strcpy(hex_row, " ");
		for(j = 0; j < 16 && (i+j) < dump_len; j++)
		{
			sprintf(hex_byte, "%02X ", (unsigned char)ca_cert_pem[i+j]);
			strcat(hex_row, hex_byte);
		}
			
		//log this row
		if(logFunc) logFunc(hex_row);
	}
		
	//add an empty line after the hex dump
	if(logFunc) logFunc("");
	
	/* This is the actual parsing of the certificate
	You need to include the null terminator */
	ret = mbedtls_x509_crt_parse(&state->cacert,
								(const unsigned char *)ca_cert_pem,
								ca_cert_len +1); //include the null terminator
	
	if(ret < 0)
	{
		mbedtls_strerror(ret, error_buf, sizeof(error_buf));
		if(logFunc)
		{
			sprintf(msg, "Failed to parse CA certificate: %s (code %d)", error_buf, ret);
			logFunc(msg);
			
			// Provide details on specific errors 
			/*
			//if(ret == MBEDTLS_ERR_PEM_NO_HEADER_FOOTER_PRESENT)
			{
				logFunc("Error detial PEM header/footer missing or invalid");
			} else if (ret == MBEDTLS_ERR_PEM_INVALID_DATA) {
				logFunc("Error detail: PEM data is not valid base64");
			} else if (ret == MBEDTLS_ERR_BASE64_INALID_CHARACTER) {
				logFunc("Error detail: Invalid base64 character found");
			} */
		}
		return ret;
	}
	
	// configure certificate
	if(logFunc) logFunc("CA certificate parsed successfully");
	mbedtls_ssl_conf_ca_chain(&state->conf, &state->cacert, NULL);
	return 0;
}

int ssl_verify_callback(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
	char buf[1024];
	char msg[1000];
	LoggingCallback logFunc = (LoggingCallback)data;
	
	//format verification issue for debugging
	mbedtls_x509_crt_verify_info(buf, sizeof(buf), " ! ", *flags);
	
	if(logFunc)
	{
		sprintf(msg, "Certificate verification issue at depth %d: %s", depth, buf);
		logFunc(msg);
	}
	return 0;
}

int custom_ssl_conf_dh_param(mbedtls_ssl_config *conf, 
						mbedtls_mpi *P, mbedtls_mpi *G)
{
/*
	if(conf->dhm.P.p != NULL)
	{
		mbedtls_mpi_free(&conf->dhm_P);
		mbedtls_mpi_free(&conf->dhm_G);
	}
	
	return mbedtls_mpi_copy(&conf->dhm_P, P) ||
			mbedtls_mpi_copy(&conf->dhm_G, G);
*/		
}