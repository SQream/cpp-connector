# SQream CPP Connector

- [Requirements](#Requirements)
  * [OpenSSL](#OpenSSL)
    + [Windows](#Windows)
- [Compiling](#Compiling)
  * [Command Line](#Command-Line])
    + [Windows](#Windows)
    + [Linux](#Linux)
  * [CMake](#CMake)
  * [Compile Requirements](#Compile-Requirements)
    + [Clang 10](#Clang-10)
- [Usage](#Usage)




## Requirements

### OpenSSL
OpenSSL 1.1.1+ is used on all platforms

#### Windows
For an easy install, you can use [Win64 OpenSSL package](https://slproweb.com/products/Win32OpenSSL.html) (the regular, not the light version)


## Compiling

Compile directly from the command line, or using [CMake](https://cmake.org/download/) (3.17.1+) and [ninja](https://ninja-build.org) (1.10.0+)

### Command Line

#### Windows

##### Build connector as a library (dll):

`clang++ -fPIC -std=c++2a  -I X:\..\OpenSSL\include -L X:\..\OpenSSL\lib\*.lib -c connector.cpp`

where -I, -L designate the path to OpenSSL .h folder and .lib files

##### Generate sample.exe:

`clang++ -std=c++2a -fuse-ld=lld -I X:\..\OpenSSL\include -L X:\..\OpenSSL\lib\*.lib -o sample.exe sample.cpp connector.cpp`

#### Linux

##### Build connector as a library (`-stdlib=libc++` may be ommitted if appropriate `stdlibc++` is present):

`clang++-10 -fPIC -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`

##### Generate sample executable (`-stdlib=libc++`, `-fuse-ld=lld` may be ommitted if appropriate `g++` toolchain is present):

`clang++-10 -std=c++2a -stdlib=libc++ -fuse-ld=lld -o sample sample.cpp connector.cpp -pthread -lssl -lcrypto`

### CMake

CMake (3.17.1+) build is tested with ninja as a backend on all platforms. To use with ninja, place the single [Windows binary](https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-win.zip) or [Linux binary](https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip) anywhere in your path.

Sample setup, from the connector folder:

```bash
mkdir build
cd build
cmake -GNinja ..
ninja
```
### Compile Requirements

#### Clang 10

##### Windows

[Clang 10](https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/LLVM-10.0.0-win64.exe), `MSVC 16.6`, `Windows 10 SDK`

MS items available via [Visual Studio Build tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)

##### Linux

`clang-10`, `libc++-10-dev`, `libc++abi-10-dev` (or equivalent level `libstc++` libraries)

###### Ubuntu

https://apt.llvm.org/  (Under `Install (stable branch)`)

##### Centos

## Usage

Put those files in the same dir and include `connector.h`:

```
connector.h
connector.cpp
socket.hpp
json.hpp
```

## Next
- Mod CMake on Linux / Windows
- Convert tests to Catch2
- Merge to hpp
- Clean up socket file
