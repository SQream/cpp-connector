## Current build

### Linux:

##### Requires: 

`libc++-10-dev`, `libc++abi-10-dev`

Ubuntu:
https://apt.llvm.org/

Centos:

##### Compilation command:
`clang++-10 -fPIC -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`

### Windows:

`clang++ -fPIC -std=c++2a -c connector.cpp socket.cpp`


## Next:
- Check ssl linkage on Windows
- Put SSL binaries / code under SSL (or use conan?)
- Convert socket to hpp / try kissnet
