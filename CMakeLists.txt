# MacSSL - MbedTLS port for Classic Mac OS using Retro68
# Ported from CodeWarrior Pro 4 to Retro68 GCC toolchain
#
# To build:
# Set RETRO68_BUILD_ROOT environment variable (or use build.sh script):
# export RETRO68_BUILD_ROOT=/path/to/your/Retro68-build
# ./build.sh
#
# Or manually:
# mkdir build && cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=$RETRO68_BUILD_ROOT/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.5)

project(MacSSL LANGUAGES C)

# Set C standard - Retro68/GCC supports C99 unlike CodeWarrior
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories for headers
include_directories(
    src/ssl                    # SSL/TLS library headers
    src/network               # Network abstraction headers
    src/demo                  # Demo application headers
    src/http                  # HTTP client library
    src/transport             # Transport layer
    src/common                # Common networking types
    src/mbedtls/include       # MbedTLS library headers from submodule
)

# MbedTLS core library source files (from submodule)
# Using the same files that were previously working
set(MBEDTLS_SOURCES
    src/mbedtls/library/aes.c
    src/mbedtls/library/aesni.c
    src/mbedtls/library/asn1parse.c
    src/mbedtls/library/asn1write.c
    src/mbedtls/library/base64.c
    src/mbedtls/library/bignum.c
    src/mbedtls/library/ccm.c
    src/mbedtls/library/certs.c
    src/mbedtls/library/cipher.c
    src/mbedtls/library/cipher_wrap.c
    src/mbedtls/library/constant_time.c
    src/mbedtls/library/ctr_drbg.c
    src/mbedtls/library/debug.c
    src/mbedtls/library/des.c
    src/mbedtls/library/dhm.c
    src/mbedtls/library/ecdh.c
    src/mbedtls/library/ecdsa.c
    src/mbedtls/library/ecp.c
    src/mbedtls/library/ecp_curves.c
    src/mbedtls/library/entropy.c
    src/mbedtls/library/error.c
    src/mbedtls/library/gcm.c
    src/mbedtls/library/md.c
    src/mbedtls/library/md5.c
    src/mbedtls/library/memory_buffer_alloc.c
    src/mbedtls/library/oid.c
    src/mbedtls/library/pem.c
    src/mbedtls/library/pk.c
    src/mbedtls/library/pk_wrap.c
    src/mbedtls/library/pkparse.c
    src/mbedtls/library/platform.c
    src/mbedtls/library/platform_util.c
    src/mbedtls/library/rsa.c
    src/mbedtls/library/rsa_internal.c
    src/mbedtls/library/sha1.c
    src/mbedtls/library/sha256.c
    src/mbedtls/library/sha512.c
    src/mbedtls/library/ssl_cache.c
    src/mbedtls/library/ssl_ciphersuites.c
    src/mbedtls/library/ssl_cli.c
    src/mbedtls/library/ssl_cookie.c
    src/mbedtls/library/ssl_msg.c
    src/mbedtls/library/ssl_srv.c
    src/mbedtls/library/ssl_ticket.c
    src/mbedtls/library/ssl_tls.c
    src/mbedtls/library/version.c
    src/mbedtls/library/version_features.c
    src/mbedtls/library/x509.c
    src/mbedtls/library/x509_create.c
    src/mbedtls/library/x509_crl.c
    src/mbedtls/library/x509_crt.c
    src/mbedtls/library/x509_csr.c
    src/mbedtls/library/x509write_crt.c
    src/mbedtls/library/x509write_csr.c
    src/mbedtls/library/xtea.c
)

# URL parsing library
set(YUAREL_SOURCES
    src/libyuarel/yuarel.c        # URL parsing library
)

# coreHTTP library source files
set(COREHTTP_SOURCES
    src/coreHTTP/source/core_http_client.c                        # Main coreHTTP client
    src/coreHTTP/source/dependency/3rdparty/llhttp/src/api.c      # llhttp API
    src/coreHTTP/source/dependency/3rdparty/llhttp/src/http.c     # llhttp HTTP handling
    src/coreHTTP/source/dependency/3rdparty/llhttp/src/llhttp.c   # llhttp parser
)

# Application source files (full implementation)
set(APP_SOURCES
    src/demo/Main.c               # Main application
    src/http/HTTPClient.c         # HTTP client implementation
    src/transport/MacHTTPTransport.c # coreHTTP transport interface for Mac
    src/ssl/SSLWrapper.c          # SSL wrapper implementation
    src/ssl/MacEntropy.c          # Classic Mac OS entropy source
    src/common/MacPlatform.c      # GCC-compatible platform compatibility layer
    src/network/MacNetwork.c      # OpenTransport networking
)

# Create the application using Retro68's add_application function
add_application(MacSSL
    ${APP_SOURCES}
    ${MBEDTLS_SOURCES}
    ${YUAREL_SOURCES}
    ${COREHTTP_SOURCES}
    TYPE APPL
    CREATOR '64bP'  # Custom creator code for 640by480 app
)

# Set target-specific include directories with proper order
target_include_directories(MacSSL BEFORE PRIVATE
    src/ssl                    # Our custom config.h first
    src/network               # Network abstraction headers
    src/demo                  # Demo application headers
    src/http                  # HTTP client library
    src/transport             # Transport layer
    src/common                # Common networking types
    src/libyuarel             # URL parsing library headers
    src/coreHTTP/source/include                                   # coreHTTP headers
    src/coreHTTP/source/interface                                 # coreHTTP transport interface
    src/coreHTTP/source/dependency/3rdparty/llhttp/include        # llhttp headers
    src/mbedtls/include       # MbedTLS library headers last
)

# Compiler optimizations and flags for Classic Mac OS
set_target_properties(MacSSL PROPERTIES
    COMPILE_OPTIONS "-ffunction-sections;-Os;-DTARGET_68K=1"
)

# Linker flags to minimize size and handle 68K specifics
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(MacSSL PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-strip-macsbug"
    )
else()
    set_target_properties(MacSSL PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections"
    )
endif()

# Add preprocessor definitions for the port
target_compile_definitions(MacSSL PRIVATE
    MBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_SOURCE_DIR}/src/ssl/mbedtls_config.h"
    MAC_OS_CLASSIC=1
    RETRO68_BUILD=1
)

# Link OpenTransport libraries using the Retro68 method
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    target_link_libraries(MacSSL -lOpenTransport -lOpenTransportApp -lOpenTptInet)
endif()