CC = g++
CC_FLAG = -std=c++11 -Wall

INC = -I/usr/local/include
# LIB = -lpthread -pthread -L/usr/local/lib -lgrpc++ -lprotobuf -lgrpc 
LIB =  -pthread -lbrpc -lprotobuf -lbrpc -L/usr/local/lib

CPPS = benchmark.pb.cc 

all: generate build 

build: 
	$(CC) $(CC_FLAG) -o server server.cc $(CPPS) $(INC) $(LIB)
	$(CC) $(CC_FLAG) -o client client.cc $(CPPS) $(INC) $(LIB) 

generate: benchmark.proto
	protoc -I. --cpp_out=. benchmark.proto

clean:
	rm -rf *.pb.* client server 