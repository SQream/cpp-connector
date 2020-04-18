CC=g++
CC_FLAGS=-fPIC -Wall -Wextra -std=c++11

ifeq ($(DEBUG),1)
	CCFLAGS+=-g
else
	CCFLAGS+=-O3 -flto
endif

all: SocketClient.o

SocketClient.o: SocketClient.hpp SocketClient.cpp SocketClient.hxx
	$(CC) $(CC_FLAGS) -c SocketClient.cpp -o SocketClient.o

clean:
	rm -rf SocketClient.o
