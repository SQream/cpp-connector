##Current build

### Linux:
Requires: libc++-10-dev libc++abi-10-dev
For ubuntu:
https://apt.llvm.org/

`clang++-10 -std=c++2a -stdlib=libc++ -c connector.cpp socket.cpp`