# SQream CPP Connector

## Requirements

### OpenSSL
OpenSSL 1.1.1+ is used on all platforms

#### Windows
For an easy install, you can use [Win64 OpenSSL package](https://slproweb.com/products/Win32OpenSSL.html) (the regular, not the light version)


### Header-Only Usage
You can include `connector.h` in your code without compiling, by placing these files with your code - 

```
connector.h
connector.cpp
socket.hpp
json.hpp
```

This usgae type is tested with Clang 10

#### Linux

`clang-10`, `libc++-10-dev`, `libc++abi-10-dev`

##### Ubuntu

https://apt.llvm.org/  (Under `Install (stable branch)`)

##### Centos

#### Windows

[Clang 10](https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/LLVM-10.0.0-win64.exe), `MSVC 16.6`, `Windows 10 SDK`

MS items available via: 

[Visual Studio Build tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)


## Compiling

Compile directly from the command line, or using [CMake](https://cmake.org/download/) (3.17.1+) and [ninja](https://ninja-build.org) (1.10.0+)

### Command Line

#### Linux

`clang++-10 -fPIC -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`

#### Windows

`clang++ -fPIC -std=c++2a  -I X:\..\OpenSSL\include -L X:\..\OpenSSL\lib\*.lib -c connector.cpp socket.cpp`

where -I, -L designate the path to OpenSSL .h folder and .lib files

### CMake

CMake build is tested with ninja (([Windows](https://github.com/ninja-build/ninja/releases), [Linux](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages)) as a backend.

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
- Clean up socket file
