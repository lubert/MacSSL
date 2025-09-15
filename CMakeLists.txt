# MacSSL - MbedTLS port for Classic Mac OS using Retro68
# Ported from CodeWarrior Pro 4 to Retro68 GCC toolchain
#
# To build:
# Set RETRO68_ROOT environment variable (or use build.sh script):
# export RETRO68_ROOT=/path/to/your/Retro68-build
# ./build.sh
#
# Or manually:
# mkdir build && cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=$RETRO68_ROOT/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.5)

project(MacSSL LANGUAGES C)

# Set C standard - Retro68/GCC supports C99 unlike CodeWarrior
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories for headers
include_directories(
    .                          # Root directory for new compatibility headers
    "Archive Folder"           # Main application headers
    mbedtls/include            # MbedTLS library headers from submodule
)

# MbedTLS core library source files (from submodule)
# Using the same files that were previously working
set(MBEDTLS_SOURCES
    mbedtls/library/aes.c
    mbedtls/library/aesni.c
    mbedtls/library/asn1parse.c
    mbedtls/library/asn1write.c
    mbedtls/library/base64.c
    mbedtls/library/bignum.c
    mbedtls/library/ccm.c
    mbedtls/library/certs.c
    mbedtls/library/cipher.c
    mbedtls/library/cipher_wrap.c
    mbedtls/library/constant_time.c
    mbedtls/library/ctr_drbg.c
    mbedtls/library/debug.c
    mbedtls/library/des.c
    mbedtls/library/dhm.c
    mbedtls/library/ecdh.c
    mbedtls/library/ecdsa.c
    mbedtls/library/ecp.c
    mbedtls/library/ecp_curves.c
    mbedtls/library/entropy.c
    mbedtls/library/error.c
    mbedtls/library/md.c
    mbedtls/library/md5.c
    mbedtls/library/memory_buffer_alloc.c
    mbedtls/library/oid.c
    mbedtls/library/pem.c
    mbedtls/library/pk.c
    mbedtls/library/pk_wrap.c
    mbedtls/library/pkparse.c
    mbedtls/library/platform.c
    mbedtls/library/platform_util.c
    mbedtls/library/rsa.c
    mbedtls/library/rsa_internal.c
    mbedtls/library/sha1.c
    mbedtls/library/sha256.c
    mbedtls/library/sha512.c
    mbedtls/library/ssl_cache.c
    mbedtls/library/ssl_ciphersuites.c
    mbedtls/library/ssl_cli.c
    mbedtls/library/ssl_cookie.c
    mbedtls/library/ssl_msg.c
    mbedtls/library/ssl_srv.c
    mbedtls/library/ssl_ticket.c
    mbedtls/library/ssl_tls.c
    mbedtls/library/version.c
    mbedtls/library/version_features.c
    mbedtls/library/x509.c
    mbedtls/library/x509_create.c
    mbedtls/library/x509_crl.c
    mbedtls/library/x509_crt.c
    mbedtls/library/x509_csr.c
    mbedtls/library/x509write_crt.c
    mbedtls/library/x509write_csr.c
    mbedtls/library/xtea.c
    cipher_stubs.c             # Stub implementations for disabled ciphers
)

# Application source files (full implementation)
set(APP_SOURCES
    Main_retro68.c             # Retro68-compatible main application
    SSLWrapper_retro68.c       # Retro68-compatible SSL wrapper
    "Archive Folder/mac_entropy.c"
    retro68_compat.c           # New GCC-compatible compatibility layer
)

# Create the application using Retro68's add_application function
add_application(MacSSL
    ${APP_SOURCES}
    ${MBEDTLS_SOURCES}
    TYPE APPL
    CREATOR '64bP'  # Custom creator code for 640by480 app
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
    MBEDTLS_CONFIG_FILE="config_minimal.h"
    MAC_OS_CLASSIC=1
    RETRO68_BUILD=1
)

# Link OpenTransport libraries using the Retro68 method
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    target_link_libraries(MacSSL -lOpenTransport -lOpenTransportApp -lOpenTptInet)
endif()