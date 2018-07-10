#include "benchmark.grpc.pb.h"

#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>

#include <iostream>
#include <thread>

using namespace std;

class HelloServiceImpl final : public grpc_benchmark::Hello::Service
{
  public:
    HelloServiceImpl(long delay) : delay(delay){}

    ::grpc::Status Say(::grpc::ServerContext *context,
                       const ::grpc_benchmark::BenchmarkMessage *request,
                       ::grpc_benchmark::BenchmarkMessage *response) override
    {
        response->CopyFrom(*request);
        response->set_field1("OK");
        response->set_field2(100);
        if (delay > 0)
        {
            this_thread::sleep_for(chrono::milliseconds(delay));
        }
        return ::grpc::Status::OK;
    }

  private:
    long delay;
};

void RunServer(string addr, long delay)
{
    
    HelloServiceImpl service(delay);

    ::grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    unique_ptr<::grpc::Server> server(builder.BuildAndStart());
    cout << "Server listening on " << addr << endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char *argv[])
{
    if(argc !=2){
        cout << "invalid param" << endl;
        return -1;
    }
    RunServer("localhost:9091", atol(argv[1]));
    return 0;
}
