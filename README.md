# SQream CPP Connector

## Requirements

### OpenSSL
OpenSSL 1.1.1+ is used on all platforms

#### Windows
For an easy install, you can use [Win64 OpenSSL package](https://slproweb.com/products/Win32OpenSSL.html) (the regular, not the light version)


### Usage

Put those files in the same dir and include `connector.h`:

```
connector.h
connector.cpp
socket.hpp
json.hpp
```

#### Linux

`clang-10`, `libc++-10-dev`, `libc++abi-10-dev` (or equivalent level `libstc++` libraries)

##### Ubuntu

https://apt.llvm.org/  (Under `Install (stable branch)`)

##### Centos

#### Windows

[Clang 10](https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/LLVM-10.0.0-win64.exe), `MSVC 16.6`, `Windows 10 SDK`

MS items available via [Visual Studio Build tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)


## Compiling

Compile directly from the command line, or using [CMake](https://cmake.org/download/) (3.17.1+) and [ninja](https://ninja-build.org) (1.10.0+)

### Command Line

#### Linux

Build connector as library (`-stdlib=libc++` may be ommitted if appropriate `stdlibc++` is present):

`clang++-10 -fPIC -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`

Generate sample executable (`-stdlib=libc++`, `-fuse-ld=lld` may be ommitted if appropriate `g++` toolchain is present):

`clang++-10 -std=c++2a -stdlib=libc++ -fuse-ld=lld -o sample sample.cpp connector.cpp -pthread -lssl -lcrypto`

#### Windows

`clang++ -fPIC -std=c++2a  -I X:\..\OpenSSL\include -L X:\..\OpenSSL\lib\*.lib -c connector.cpp socket.cpp`

where -I, -L designate the path to OpenSSL .h folder and .lib files

### CMake

CMake (3.17.1+) build is tested with ninja as a backend on all platforms. To use with ninja, place the single [Windows binary](https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-win.zip) or [Linux binary](https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip) anywhere in your path.

Sample setup, from the connector folder:

```bash
mkdir build
cd build
cmake -GNinja ..
ninja
```

## Next
- Mod CMake on Linux / Windows
- Convert tests to Catch2
- Merge to hpp
- Clean up socket file
