CMAKE_MINIMUM_REQUIRED(VERSION 3.17.1)
set(CMAKE_CXX_STANDARD 20)

project(connector LANGUAGES CXX DESCRIPTION "SQream C++ connector" HOMEPAGE_URL https://github.com/SQream/cpp-connector)

set(CMAKE_CXX_COMPILER "clang++")

# #[[
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings 
    add_compile_options(-Wall -Wextra -pedantic)  # -Werror - all warnings as errors
endif()
#]]

# set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Set -fPIC on all targets

include_directories (.)

find_package(OpenSSL REQUIRED)

add_library(connector ./connector.cpp)
set_property(TARGET connector PROPERTY POSITION_INDEPENDENT_CODE ON)       # -fPIC 
set_property(TARGET connector PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)  # -flto 



target_link_libraries(connector PRIVATE OpenSSL::SSL OpenSSL::Crypto)
