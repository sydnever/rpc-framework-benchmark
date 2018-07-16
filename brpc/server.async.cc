// A server to receive EchoRequest and send back EchoResponse.
#include "benchmark.pb.h"

#include <butil/logging.h>
#include <brpc/server.h>

#include <thread>
#include <chrono>

using namespace std;

// Your implementation of example::EchoService
// Notice that implementing brpc::Describable grants the ability to put
// additional information in /status.

class HelloImpl final : public brpc_benchmark::Hello
{
  public:
    HelloImpl(int delay) : delay(delay){};
    ~HelloImpl(){};
    void Say(google::protobuf::RpcController *cntl_base,
             const brpc_benchmark::BenchmarkMessage *request,
             brpc_benchmark::BenchmarkMessage *response,
             google::protobuf::Closure *done)
    {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        brpc::ClosureGuard done_guard(done);

        brpc::Controller *cntl =
            static_cast<brpc::Controller *>(cntl_base);

        // Fill response.
        if (delay > 0)
        {
            this_thread::sleep_for(chrono::milliseconds(delay));
        }
        response->CopyFrom(*request);
        response->set_field1("OK");
        response->set_field2(100);
    }

  private:
    int delay;
};

int main(int argc, char *argv[])
{
    int delay;
    if (argc != 2)
        delay = 0;
    else
        delay = atoi(argv[1]);

    // Generally you only need one Server.
    brpc::Server server;

    // Instance of your service.
    HelloImpl hello_impl(delay);

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&hello_impl,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
    {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }
    server.MaxConcurrencyOf(&hello_impl, "Say") = 10000000;
    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    if (server.Start(9092, &options) != 0)
    {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
