# MacSSL Demo Application
# Builds the demo application that uses the MacSSL library
#
# To build this demo as a standalone project:
# mkdir build
# cd build
# cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/Retro68-build/toolchain/cmake/retro68.toolchain.cmake
# make

cmake_minimum_required(VERSION 3.5)

project(MacSSLDemo LANGUAGES C)

# Set C standard - Retro68/GCC supports C99 unlike CodeWarrior
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find the MacSSL library (assumes it's built in ../build)
# If building standalone, you would need to build the library first
set(MACSSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Include the library's CMakeLists to get the library target
add_subdirectory(${MACSSL_ROOT_DIR} ${CMAKE_CURRENT_BINARY_DIR}/macssl_lib)

# Demo application source files
set(DEMO_SOURCES
    Main.c                        # Main demo application
    DemoHTTP.c                    # Demo-specific HTTP functions
)

# Create the demo application using Retro68's add_application function
add_application(PostMac
    ${DEMO_SOURCES}
    TYPE APPL
    CREATOR 'PosM'  # Custom creator code for PostMac app
)

# Link against the MacSSL library
target_link_libraries(PostMac MacSSL)

# Add demo-specific include directories
target_include_directories(PostMac PRIVATE
    .                             # Demo directory for local headers
    ../src/ssl                    # For demo to access library headers directly
    ../src/network                # Network headers for demo
    ../src/http                   # HTTP headers for demo
    ../src/common                 # Common headers for demo
    ../src/libyuarel              # URL parsing for demo
)

# Compiler optimizations and flags for Classic Mac OS demo
set_target_properties(PostMac PROPERTIES
    COMPILE_OPTIONS "-ffunction-sections;-Os;-DTARGET_68K=1"
)

# Linker flags to minimize size and handle 68K specifics
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(PostMac PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-strip-macsbug"
    )
else()
    set_target_properties(PostMac PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections"
    )
endif()