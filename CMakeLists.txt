cmake_minimum_required(VERSION 3.12)
project(SQream-cpp-connector)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(DIST_VER "system-default")

set(CMAKE_CXX_FLAGS "-fPIC -Wall -Wextra -shared-libgcc -g3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG")

set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/sqream-prerequisites/versions/${DIST_VER}/include)

link_directories(/usr/local/sqream-prerequisites/versions/${DIST_VER}/lib64 /usr/local/sqream-prerequisites/versions/${DIST_VER}/lib)

add_library(sqream SHARED SQream-cpp-connector.cc)

add_executable(runtime_test runtime_test.cc SQream-cpp-connector.cc SOCK/linuxSock/SocketClient.cpp)

target_link_libraries(runtime_test crypto ssl boost_unit_test_framework pthread)

