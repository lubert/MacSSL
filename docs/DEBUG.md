# MacSSL Debug Guide

## SSL Handshake RSA Padding Error (-16640) Fix

### Problem Description
The MacSSL application was failing with RSA invalid padding error (code -16640 / 0xffffbf00) during SSL handshake with modern HTTPS servers like 640by480.com.

### Root Cause
**Not certificate issues** - The real problem was TLS version and RSA signature algorithm incompatibility:

1. **TLS 1.3 Negotiation**: Modern servers negotiate TLS 1.3 by default
2. **RSA-PSS Signatures**: TLS 1.3 uses RSA-PSS signatures which require PKCS#1 v2.1 padding
3. **Classic Mac OS Limitation**: MbedTLS on 68K only supports PKCS#1 v1.5 padding, not RSA-PSS

### Solution Applied

#### 1. MbedTLS Configuration (`config_minimal.h`)
```c
/* Force TLS 1.2 maximum for Classic Mac OS compatibility */
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_MAX_MAJOR_VERSION MBEDTLS_SSL_MAJOR_VERSION_3
#define MBEDTLS_SSL_MAX_MINOR_VERSION MBEDTLS_SSL_MINOR_VERSION_3

/* RSA settings - Only PKCS#1 v1.5, no RSA-PSS */
#define MBEDTLS_PKCS1_V15
#undef MBEDTLS_PKCS1_V21  /* Disable RSA-PSS padding */

/* Increased memory allocation for larger responses */
#define MBEDTLS_SSL_MAX_CONTENT_LEN 16384
#define MBEDTLS_MPI_MAX_SIZE 1024
```

#### 2. Debug Support Disabled
```c
/* Remove debug support to prevent crashes from excessive logging */
/* #define MBEDTLS_DEBUG_C */
```

#### 3. Display Buffer Limits (`Main_retro68.c`)
```c
/* Display only first 512 bytes to avoid GUI crashes */
sprintf(statusMsg, "JSON response received: %ld bytes (not displayed to prevent crashes)", bodyLength);
```

### Testing Results
- ✅ SSL handshake: `TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256` on TLS 1.2
- ✅ Certificate verification: Complete Let's Encrypt chain validation
- ✅ Data transfer: Successfully receives 8KB+ JSON responses
- ✅ Stability: No crashes during operation

### Key Insights
1. **Certificates were never the problem** - Original certificate chain was fine
2. **RSA padding error occurred during handshake**, not certificate validation
3. **TLS version negotiation** was the real culprit
4. **Classic Mac OS GUI limitations** require careful buffer management

### Configuration Files Modified
- `config_minimal.h` - MbedTLS configuration (primary fix)
- `Main_retro68.c` - Display buffer limits (stability fix)

### Error Codes Reference
- `-16640` (0xffffbf00) = `MBEDTLS_ERR_RSA_INVALID_PADDING`
- Common with RSA-PSS vs PKCS#1 v1.5 incompatibility

### Compatibility Notes
- **Works with**: TLS 1.2 servers using ECDHE-RSA + AES-CBC + SHA256
- **Requires**: PKCS#1 v1.5 signature padding (not RSA-PSS)
- **Memory**: 16KB SSL buffers, 1KB MPI buffers
- **Display**: Limited to small text snippets to prevent GUI crashes

### Future Debugging
If SSL issues reoccur:
1. Check TLS version negotiation first
2. Verify RSA signature algorithm compatibility
3. Monitor memory usage during large transfers
4. Test GUI display limits with response data

---
*Last updated: September 2025*
*Classic Mac OS HTTPS client using Retro68 + MbedTLS*