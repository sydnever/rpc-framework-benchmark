CC = g++
CC_FLAG = -std=c++11 -Wall -O2

INC = -I/usr/local/include
# LIB = -lpthread -pthread -L/usr/local/lib -lgrpc++ -lprotobuf -lgrpc 
LIB =  -pthread -lbrpc -lprotobuf -lbrpc -L/usr/local/lib

CPPS = benchmark.pb.cc 

all: generate build 

build: 
	$(CC) $(CC_FLAG) -o server server.cc $(CPPS) $(INC) $(LIB)
	$(CC) $(CC_FLAG) -o client client.cc $(CPPS) $(INC) $(LIB) 
	$(CC) $(CC_FLAG) -o server.async server.async.cc $(CPPS) $(INC) $(LIB)
	$(CC) $(CC_FLAG) -o client.async client.async.cc $(CPPS) $(INC) $(LIB) 
	$(CC) $(CC_FLAG) -o client.async2 client.async2.cc $(CPPS) $(INC) $(LIB) 
	
	

build1:
	$(CC) $(CC_FLAG) -o server.1 server.1.cc $(CPPS) $(INC) $(LIB)
	$(CC) $(CC_FLAG) -o client.1 client.1.cc $(CPPS) $(INC) $(LIB) 

generate: benchmark.proto
	protoc -I. --cpp_out=. benchmark.proto

clean:
	rm -rf *.pb.* client server 