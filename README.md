## Current build

### Requirements:

#### Linux: 

`clang-10`, `libc++-10-dev`, `libc++abi-10-dev`

##### Ubuntu:

https://apt.llvm.org/  (Under `Install (stable branch)`)

##### Centos:


#### Windows:

[Clang 10](https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/LLVM-10.0.0-win64.exe), `MSVC 16.6`, `Windows 10 SDK`

MS items available via: 

[Visual Studio Build tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)


### Compilation:
#### Linux: 

`clang++-10 -fPIC -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`

#### Windows:

`clang++ -fPIC -std=c++2a -c connector.cpp socket.cpp`


## Next:
- Check ssl linkage on Windows
- Put SSL binaries / code under SSL (or use conan?)
- Convert socket to hpp / try kissnet
