#include "benchmark.pb.h"

#include <brpc/channel.h>
#include <brpc/server.h>
#include <bthread/bthread.h>
#include <butil/logging.h>
#include <bvar/bvar.h>
#include <gflags/gflags.h>

#include <atomic>
#include <iostream>
#include <string>
#include <sys/time.h>

using namespace std;

int thread_num = 50;           //"Number of threads to send requests");
bool use_bthread = false;      //"Use bthread to send requests");
int attachment_size = 0;       //"Carry so many byte attachment along with requests");
int request_size = 16;         //"Bytes of each request");
string protocol = "baidu_std"; //"Protocol type.  in src =brpc/options.proto");
string connection_type =
    "";                         //"Connection type. Available values: single, pooled, short");
string server = "0.0.0.0:9092"; //"IP Address of server");
string load_balancer = "";      //"The algorithm for load balancing");
int timeout_ms = 100;           //"RPC timeout in milliseconds");
int max_retry = 3;              //"Max retries(not including the first RPC)");
bool dont_fail = false;         //"Print fatal when some call failed");
bool enable_ssl = false;        //"Use SSL connection");
int dummy_port = -1;            //"Launch dummy server at this port");
string http_content_type =
    "application/json"; //"Content type of http request");

std::string g_request;
std::string g_attachment;

bvar::LatencyRecorder g_latency_recorder("client");
bvar::Adder<int> g_error_count("client_error_count");

brpc_benchmark::BenchmarkMessage prepare_args()
{
    brpc_benchmark::BenchmarkMessage msg;
    bool b = true;
    int i = 100000;
    string str = "许多往事在眼前一幕一幕，变的那麼模糊";
    msg.set_field1(str);
    msg.set_field2(i);
    msg.set_field3(i);
    msg.set_field4(str);
    msg.set_field5(i);
    msg.set_field6(i);
    msg.set_field7(str);
    msg.set_field9(str);
    msg.set_field12(b);
    msg.set_field13(b);
    msg.set_field14(b);
    msg.set_field16(i);
    msg.set_field17(b);
    msg.set_field18(str);
    msg.set_field22(i);
    msg.set_field23(i);
    msg.set_field24(b);
    msg.set_field25(i);
    msg.set_field29(i);
    msg.set_field30(b);
    msg.set_field59(b);
    msg.set_field60(i);
    msg.set_field67(i);
    msg.set_field68(i);
    msg.set_field78(b);
    msg.set_field80(b);
    msg.set_field81(b);
    msg.set_field100(i);
    msg.set_field101(i);
    msg.set_field102(str);
    msg.set_field103(str);
    msg.set_field104(i);
    msg.set_field128(i);
    msg.set_field129(str);
    msg.set_field130(i);
    msg.set_field131(i);
    msg.set_field150(i);
    msg.set_field271(i);
    msg.set_field272(i);
    msg.set_field280(i);

    return msg;
}

void HandleEchoResponse(brpc::Controller *cntl,
                        brpc_benchmark::BenchmarkMessage *response)
{
    // std::unique_ptr makes sure cntl/response will be deleted before returning.
    std::unique_ptr<brpc::Controller> cntl_guard(cntl);
    std::unique_ptr<brpc_benchmark::BenchmarkMessage> response_guard(response);

    if (cntl->Failed())
    {
        LOG(WARNING) << "Fail to send EchoRequest, " << cntl->ErrorText();
        return;
    }
}

static void *sender(void *arg)
{
    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    brpc_benchmark::Hello_Stub stub(
        static_cast<google::protobuf::RpcChannel *>(arg));

    while (!brpc::IsAskedToQuit())
    {
        // We will receive response synchronously, safe to put variables
        // on stack.
        brpc_benchmark::BenchmarkMessage request;
        brpc_benchmark::BenchmarkMessage *response =
            new brpc_benchmark::BenchmarkMessage();
        brpc::Controller *cntl = new brpc::Controller();

        request.CopyFrom(prepare_args());

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        google::protobuf::Closure *done =
            brpc::NewCallback(&HandleEchoResponse, cntl, response);
        stub.Say(cntl, &request, response, done);

        // if (!cntl->Failed())
        // {
        //     g_latency_recorder << cntl->latency_us();
        // }
        // else
        // {
        //     g_error_count << 1;
        //     CHECK(brpc::IsAskedToQuit() || !dont_fail)
        //         << "error=" << cntl->ErrorText() << " latency=" << cntl->latency_us();
        //     // We can't connect to the server, sleep a while. Notice that this
        //     // is a specific sleeping to prevent this thread from spinning too
        //     // fast. You should continue the business logic in a production
        //     // server rather than sleeping.
        //     bthread_usleep(50000);
        // }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2){
        thread_num = 2; 
    }else{
        thread_num = atoi(argv[1]);
    }


    // Parse gflags. We recommend you to use gflags as well.

    // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::Channel channel;

    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.ssl_options.enable = enable_ssl;
    options.protocol = protocol;
    options.connection_type = connection_type;
    options.connect_timeout_ms = std::min(timeout_ms / 2, 100);
    options.timeout_ms = timeout_ms;
    options.max_retry = max_retry;
    if (channel.Init(server.c_str(), load_balancer.c_str(), &options) != 0)
    {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    if (attachment_size > 0)
    {
        g_attachment.resize(attachment_size, 'a');
    }
    if (request_size <= 0)
    {
        LOG(ERROR) << "Bad request_size=" << request_size;
        return -1;
    }
    g_request.resize(request_size, 'r');

    if (dummy_port >= 0)
    {
        brpc::StartDummyServerAt(dummy_port);
    }

    std::vector<bthread_t> bids;
    std::vector<pthread_t> pids;
    if (!use_bthread)
    {
        pids.resize(thread_num);
        for (int i = 0; i < thread_num; ++i)
        {
            if (pthread_create(&pids[i], NULL, sender, &channel) != 0)
            {
                LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    }
    else
    {
        bids.resize(thread_num);
        for (int i = 0; i < thread_num; ++i)
        {
            if (bthread_start_background(&bids[i], NULL, sender, &channel) != 0)
            {
                LOG(ERROR) << "Fail to create bthread";
                return -1;
            }
        }
    }

    while (!brpc::IsAskedToQuit())
    {
        sleep(1);
        LOG(INFO) << "Sending EchoRequest at qps=" << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    LOG(INFO) << "EchoClient is going to quit";
    for (int i = 0; i < thread_num; ++i)
    {
        if (!use_bthread)
        {
            pthread_join(pids[i], NULL);
        }
        else
        {
            bthread_join(bids[i], NULL);
        }
    }

    return 0;
}
