# MacSSL
## A port of Mbed-TLS for the Classic Macintosh OS 7/8/9

_Note: this repository will never change. It's a proof of concept and template._

This is a C89/C90 port of MbedTLS for Mac System 7/8/9. It works, and compiles under Metrowerks Codewarrior Pro 4. Here's the proof:

![Proof of pulling an API request down](https://bbenchoff.github.io/images/640by480Client.png)

This is a basic app that performs a GET request on whatever is in api.h, and prints the result out to the text box (with a lot of debug information, of course). The idea of this project was to build an 'app' of sorts for [640by480](https://640by480.com/), my 'instagram clone for vintage digital cameras'. The idea would be to login, post images, view images, and read comments. I would need HTTPS for that, so here we are: a port of MbedTLS for the classic mac.

## What is in this repository

This repository contains all the files in my Metrowerks Codewarrior project in the /Project folder. Ideally, if the Mac didn't have the whole 'resource problem' and the '\n \r \cr' problem, you could just download that folder to your Mac, open the project file with Metrowerks Codewarrior Pro 4, and compile the app.

But we don't live in a perfect world, and I have to deal with Mac file resources and such, so I've also compressed that folder. The _entire_ project folder, with Codewarrior project file, sources, compiler output, and polarssl library, is available as the `Archive.sit` file. This was compressed with DropStuff 4.0, and should open with any of the Stuffit tools. Download the `Archive.sit` project, unstuffit on your mac, and you'll have everything you need.

This project now uses the official Mbed-TLS library as a git submodule, replacing the previous flattened PolarSSL subset. The mbedtls submodule points to the official Mbed-TLS v2.28.9 repository.

## What this port is based on, and limitations

This port is based on [polarssl](https://github.com/cuberite/polarssl), itself a fork of [Mbed-TLS](https://github.com/Mbed-TLS/mbedtls), version 2.29.9, or thereabouts. This is a C library that implements the crypto primitives, X.509 certificate manipulation, and the SSL/TLS protocols.

Currently, the bare minimum configuration of this library supports the following:

### Ciphersuites
* `MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA`
* `MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA`

### Elliptic Curves
* `MBEDTLS_ECP_DP_SECP256R1`

### Signature Algorithms
* `SHA-256 + RSA`
* `SHA-384 + RSA`
* `SHA-1 + RSA`

### Certificate Handling
* Root cert `ISRG Root X1`
* Intermediate cert `Let's Encrypt R11`

All of this is wrapped up into support for `TLS 1.1`. This was enough for what _I_ wanted to do, but it provides a basic framework for adding `TLS 1.2`, additional ciphersuites like `ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256`, and more elliptic curves such as `ECP_DP_CURVE25519`. The framework is there, but if you want to add these it's going to take a little work.

## Building with Retro68

This project can now be built using the [Retro68](https://github.com/autc04/Retro68) GCC toolchain instead of CodeWarrior. To build:

1. Install and build Retro68 following their documentation
2. Set the `RETRO68_BUILD_ROOT` environment variable to your Retro68 build directory:
   ```bash
   export RETRO68_BUILD_ROOT=/path/to/your/Retro68-build
   ```
3. Run the build script:
   ```bash
   ./build.sh
   ```

Alternatively, you can build manually with CMake:
```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$RETRO68_BUILD_ROOT/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make
```

The Retro68 build uses the official Mbed-TLS library as a git submodule, providing better compatibility and easier maintenance compared to the original flattened PolarSSL subset.

## The Example App

This repository produces a FAT-compiled application for Mac System 7/8/9. It works through the OpenTransport library, i.e. MacTCP is not supported. This application sends a GET request to the API endpoint of a server [https://640by480.com/api/v1/posts](https://640by480.com/api/v1/posts), and returns the results to a text box, and also writes a file to the disk in the same location the application is run from. This file, `SSL-Debug.txt`, also saves the debug info from `mbedtls_debug_set_threshold()`; the value of this debug threshold can be adjusted in `SSLWrapper.c`. More info on that below.

## SSL Implementation Challenges

Mbedtls was written for C99 compilers, but my version of CodeWarrior only supports C89/C90. The transition required significant code modifications:

* Creating compatibility layers for modern C integer types
* Implementing 64-bit integer emulation
* Restructuring code to declare variables at block beginnings (C89 requirement)
* Addressing include path limitations in Mac's un-*NIX-like file system

That last bit -- addressing the path limitations -- is a big one. You know how you can write `#include "mbedtls/aes.h"`, and the compiler will pull in code from the `aes.h` file that's in the `mbedtls` folder? You can't do that on a Mac! Or at least I couldn't figure out how Codewarrior defines paths.  The solution is basically to put all the files from Mbed-TLS into the project as a flat directory.

The biggest problem? **C89 doesn't support variadic macros or method overloading. 64-bit ints are completely unknown on this platform**. If you don't know what I'm talking about, here's an example of method overloading:

```c
void print(int x);
void print(const char* s);
```

Those are two functions, both of them return nothing, but one of them takes an int, and the other a string. _They're both named the same thing_. This works if you have method overloading, like is found in C99. C89/90 doesn't have it, and it's a bitch and a half to port C89 code to C99 because of this. This also shows up in variadic macros, which I believe is a portmanteau of _variable argument_. It's something like this:

```c
#define superprint(...) fprintf(stderr, __VA_ARGS__);
```

This is a way to do something like method overloading, but using the preprocessor instead of the language itself. Obviously we see more method overloading this century simply because languages support it now, so different names for the same thing, I guess.

Yeah, this was an incredibly time consuming and boring fix.

## Entropy Collection Nightmare

I've discoverd a great plot hole in an Asimov short story. If you're wondering how can the net amount of entropy of the universe be massively decreased, the answer isn't to use a computer trillions of years in the future, the answer is to use a computer built thirty years ago.

The classic Mac OS has very little entropy, something required for high-quality randomness. This meant my SSL implementation gave the error code `MBEDTLS_ERR_ENTROPY_SOURCE_FAILED`. I created a custom entropy collection system that draws from multiple sources:

* System clock and tick counts at microsecond resolution
* Mouse movement tracking
* Memory states and allocation patterns
* Hardware timing variations
* Network packet timing with OTGetTimeStamp()
* TCP sequence numbers and connection statistics
* Time delays between user interactions
* The amount of time it takes for the screensaver to activate

All of these sources are combined and XORed together for a pool of randomness that's sufficient for crypto operations. I wouldn't exactly call this _random_, but it's random enough to initialize the crypto subsystems in mbedtls. It works, but I make no guarantees about its security of this entropy function. *This mbedtls implementation should be considered insecure*.

## Certificate Handling

Yes, this code can handle certificates. The current certificate handling is set to `OPTIONAL`, but it does work when `REQUIRED`.

The root certificate is the ISRG Root X1, and the intermediate certificate is the Let's Encrypt R11. This provides enough to connect to the end certificate for 640by480.com. Root certificate trust is stored in the code at SSLWrapper.c.

## Debug Log

As mentioned above, this app has two methods of output: it displays information (and eventually the result of the GET request) to a textbox. It also saves _everything_ to a file on disk. This bifurcation of debug information is due to the 32k limit of a TETextBox of the classic Macintosh Toolbox. This window cannot display more than 32000 characters without a bit of work, and the combination of debug information and the GET result will probably push that over the 32k limit.

## License

A large amount of the SSL code is licensed as:

``` 
Copyright The Mbed TLS Contributors
SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
```

Note that posting my modifications to this code fufills the requirements of GPL-licensed code.

The code I contributed to this is licensed as:

```
           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    MODIFIED FOR NERDS 
                   Version 3, April 2025

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.
 
           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.

 1. Anyone who complains about this license is a nerd.
```