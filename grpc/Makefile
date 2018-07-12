CC = g++
CC_FLAG = -std=c++11 -Wall -O2

INC = -I/usr/local/include
# LIB = -lpthread -pthread -L/usr/local/lib -lgrpc++ -lprotobuf -lgrpc 
LIB =  -pthread -lgrpc++ -lprotobuf -lgrpc -L/usr/local/lib

CPPS = benchmark.pb.cc benchmark.grpc.pb.cc

all: generate build 

build: 
	$(CC) $(CC_FLAG) -o server benchmark_server.cc $(CPPS) $(INC) $(LIB)
	$(CC) $(CC_FLAG) -o client benchmark_client.cc $(CPPS) $(INC) $(LIB) 

generate: benchmark.proto
	protoc -I. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` benchmark.proto
	protoc -I. --cpp_out=. benchmark.proto

clean:
	rm -rf *.pb.* client server 