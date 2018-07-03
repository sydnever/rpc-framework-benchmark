CPPS=gen-cpp/benchmark_constants.cpp gen-cpp/Greeter.cpp gen-cpp/benchmark_types.cpp

#set your own environment option
CC = g++
CC_FLAG = -std=c++11 -Wall

#set your inc and lib
INC = -I/usr/local/include/thrift
LIB = -lpthread -lthrift -L/usr/local/lib
 
C_COMMAND = $(CC) $(CC_FLAG) $(CPPS) $(INC) $(LIB)

all: generate build

generate: thrift

build: server client

thrift: benchmark.thrift
	rm -rf gen-cpp && thrift -r -gen cpp benchmark.thrift

server: $(CPPS)
	$(CC) $(CC_FLAG) -o server my_server.cpp $(CPPS) $(INC) $(LIB)

client: $(CPPS)
	$(CC) $(CC_FLAG) -o client my_client.cpp $(CPPS) $(INC) $(LIB)

clean: 
	rm -rf gen-cpp server client
