CC=g++
CC_FLAGS=-fPIC -Wall -Wextra -std=c++11
LD_LINK_EXTRA=
DIST_VER=system-default
DIST_FLAGS=-L/usr/local/sqream-prerequisites/versions/${DIST_VER}/lib -I/usr/local/sqream-prerequisites/versions/${DIST_VER}/include

ROOT_DIR:=$(shell pwd)
GIT_CURRENT_SHA1:=$(shell git rev-parse --short HEAD)
APP_MAJOR:=$(shell awk '/\#define CPPCONECTOR_MAJOR_VERSION/ { print $$3 }' SQream-cpp-connector.h)
APP_MINOR:=$(shell awk '/\#define CPPCONECTOR_MINOR_VERSION/ { print $$3 }' SQream-cpp-connector.h)
#APP_PATCH:=$(shell awk '/\#define CPPCONECTOR_PATCH_VERSION/ { print $$3 }' SQream-cpp-connector.h) --No need for use currently


ifeq ($(DEBUG),1)
	CC_FLAGS+= -g3 -O0
else
	CC_FLAGS+= -O3 -flto -DNDEBUG
endif

ifeq ($(COVERAGE),1)
	CC_FLAGS+= --coverage -DCOVERAGE
	LD_LINK_EXTRA+= --coverage
endif

all: libsqream.so.$(APP_MAJOR).$(APP_MINOR) doxygen example runtime_test network_insert_benchmark

#### This application includes all the functional tests of the driver. It uses the sources directly (not the compiled library) and thus need the dependencies (ssl, crypto)
runtime_test: SQream-cpp-connector.o runtime_test.cc
	$(CC) $(CC_FLAGS) $(DIST_FLAGS) -lcrypto -lssl -lpthread -lboost_unit_test_framework runtime_test.cc SQream-cpp-connector.o SOCK/linuxSock/SocketClient.o -o runtime_test

network_insert_benchmark: network_insert_benchmark.cpp
	$(CC) $(CC_FLAGS) $(DIST_FLAGS) -lboost_system -lboost_filesystem -lboost_program_options -L./ network_insert_benchmark.cpp -lsqream -o network_insert_benchmark

#### This is an application example that uses the compiled library that is already linked with its dependencies
example: example.cc
	$(CC) $(CC_FLAGS) -L./ example.cc -lsqream -o example

doxygen: SQream-cpp-connector.cc SQream-cpp-connector.h Doxyfile
	-doxygen Doxyfile

SQream-cpp-connector.o: SQream-cpp-connector.cc SQream-cpp-connector.h
	make -C SOCK/linuxSock/
	$(CC) $(CC_FLAGS) -c SQream-cpp-connector.cc -o SQream-cpp-connector.o

# Here are followed the conventions for creating a shared library on linux systems as described in 'the linux documentation project'
# http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
libsqream.so.$(APP_MAJOR).$(APP_MINOR): SQream-cpp-connector.o
	$(CC) $(CC_FLAGS) -lcrypto -lssl -lpthread -shared -Wl,-soname,libsqream.so.$(APP_MAJOR) -o libsqream.so.$(APP_MAJOR).$(APP_MINOR) SQream-cpp-connector.o SOCK/linuxSock/SocketClient.o
	-ln -s libsqream.so.$(APP_MAJOR).$(APP_MINOR) libsqream.so.$(APP_MAJOR)
	-ln -s libsqream.so.$(APP_MAJOR) libsqream.so
	
test-coverage: runtime_test
	-lcov --zerocounters --directory .
	lcov --no-external --capture --initial --directory . --output-file base_cpp_coverage.info
	./runtime_test
	lcov --no-external --capture --directory . --output-file test_cpp_coverage.info
	lcov --add-tracefile base_cpp_coverage.info --add-tracefile test_cpp_coverage.info --output-file combined_coverage_data.info
	lcov --remove combined_coverage_data.info "${ROOT_DIR}/example.cc" "${ROOT_DIR}/network_insert_benchmark.cpp" "${ROOT_DIR}/runtime_test.cc" "/usr/local/sqream-prerequisites/*" "${ROOT_DIR}/doxygen/*" "${ROOT_DIR}/rapidjson/*" "${ROOT_DIR}/.temp_master_clone/*" --output-file combined_coverage_data_filtered.info
	genhtml combined_coverage_data_filtered.info --highlight --legend --title "Commit: ${GIT_CURRENT_SHA1}" --output-directory combined_coverage
	gcovr  -f "${ROOT_DIR}/SQream-cpp-connector.cc" -f "${ROOT_DIR}/SQream-cpp-connector.h" --exclude "${ROOT_DIR}/example.cc" --exclude "${ROOT_DIR}/network_insert_benchmark.cpp" --exclude "${ROOT_DIR}/runtime_test.cc" --exclude "${ROOT_DIR}/.temp_master_clone" --exclude "/usr/local/sqream-prerequisites" --exclude "${ROOT_DIR}/doxygen" --exclude "${ROOT_DIR}/rapidjson" --xml-pretty --output gcovr_coverage.xml

clean-test-coverage:
	-rm -f *.info
	-rm gcovr_coverage.xml
	-lcov --zerocounters --directory .

clean: clean-test-coverage
	make clean -C SOCK/linuxSock/
	-rm -rf SocketClient.o SQream-cpp-connector.o runtime_test example network_insert_benchmark doxygen libsqream.so*
