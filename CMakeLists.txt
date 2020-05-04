CMAKE_MINIMUM_REQUIRED(VERSION 3.17.1)
set(CMAKE_CXX_STANDARD 20)

project(connector LANGUAGES CXX DESCRIPTION "SQream C++ connector" HOMEPAGE_URL https://github.com/SQream/cpp-connector)

set(CMAKE_CXX_COMPILER "clang++")
include_directories (.)

##[[
if (MSVC)
    add_compile_options(/W4)                                              # warning level 4. /WX- all warnings as errors
else()
    add_compile_options(-Wall -Wextra -pedantic)                          # lots of warnings  -Werror - all warnings as errors
endif()
#]]


## ----- linking requirements  ------

# OpenSSL
find_package(OpenSSL REQUIRED)
link_libraries(OpenSSL::SSL OpenSSL::Crypto)  # Link all targets with openssl / crypto                             

# pthreads
# set(CMAKE_THREAD_PREFER_PTHREAD ON)
# set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads) # Link all targets with thread

## ---- .so / .dll build ------
add_library(connector SHARED ./connector.cpp)                              # Sets POSITION_INDEPENDENT_CODE as well
set_property(TARGET connector PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)  # -flto 

#! set_property(TARGET connector PROPERTY POSITION_INDEPENDENT_CODE ON)    # done automatically when setting library to SHARED
#! set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Set -fPIC on all targets

## ----- sample.exe build -----
add_executable(sample ./sample.cpp ./connector.cpp)

## ----- tests.exe build -----
add_executable(sq_tests ./tests/tests.cpp ./connector.cpp)


#! Alternative for linking per target - we link ssl/crypto for all targets
#! target_link_libraries(connector PRIVATE OpenSSL::SSL OpenSSL::Crypto)
#! target_link_libraries(sample    PRIVATE OpenSSL::SSL OpenSSL::Crypto)
