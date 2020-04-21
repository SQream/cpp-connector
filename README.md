## Current build

### Linux:

##### Requires: `libc++-10-dev`, `libc++abi-10-dev`
For ubuntu:
https://apt.llvm.org/

For centos:

##### Compilation command:
`clang++-10 -fPIC -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`

### Windows:

`clang++ -fPIC -std=c++2a -c connector.cpp socket.cpp`


## Next:
- Check ssl linkage on Windows
- Put SSL binaries / code under SSL (or use conan?)
- Convert socket to hpp / try kissnet
