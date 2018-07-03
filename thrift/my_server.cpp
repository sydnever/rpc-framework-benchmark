#include "gen-cpp/Greeter.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>

#include <chrono>
#include <cstdio>
#include <thread>

#define PORT 9090

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

class GreeterHandler : virtual public GreeterIf {
public:
  long delay;
  GreeterHandler(long delay) { this->delay = delay; }

  void say(BenchmarkMessage &_return, const BenchmarkMessage &msg) {
    _return = msg;
    _return.__set_field1("OK");
    _return.__set_field2(100);
    if (delay > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
  }
};

int main(int argc, char **argv) {
  long delay = 0;
  if (argc > 1) {
    delay = atol(argv[1]);
  }

  TThreadedServer server(
    stdcxx::make_shared<GreeterProcessor>(stdcxx::make_shared<GreeterHandler>(delay)),
    stdcxx::make_shared<TServerSocket>(PORT), 
    stdcxx::make_shared<TBufferedTransportFactory>(),
    stdcxx::make_shared<TBinaryProtocolFactory>());

  server.serve();
  return 0;
}
