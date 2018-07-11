// Copyright (c) 2014 Baidu, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A client sending requests to server every 1 second.

#include "benchmark.pb.h"

#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>

#include <string>

using namespace std;

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

int main(int argc, char *argv[])
{

    string server = "127.0.0.1:9092";  // "IP Address of server");
    string load_balancer = "";  // "The algorithm for load balancing");
    int32_t timeout_ms = 100;  // "RPC timeout in milliseconds");
    int32_t max_retry = 3;  // "Max retries(not including the first RPC)");
    int32_t interval_ms = 1000;  // "Milliseconds between consecutive requests");
    string http_content_type = "application/json";  // "Content type of http request");

    string protocol = "baidu_std"; // Protocol type. Defined in src/brpc/options.proto
    string connection_type =  ""; // Connection type. Available values: single, pooled, short

    long thread_num = 1;
    long request_num = 1;
    if (argc == 3)
    {
        thread_num = atol(argv[1]);
        request_num = atol(argv[2]);
    }

    // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::Channel channel;

    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = protocol;
    options.connection_type = connection_type;
    options.timeout_ms = timeout_ms /*milliseconds*/;
    options.max_retry = max_retry;
    if (channel.Init(server.c_str(), load_balancer.c_str(), &options) != 0)
    {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    brpc_benchmark::Hello_Stub stub(&channel);

    // Send a request and wait for the response every 1 second.
    int log_id = 0;
    while (!brpc::IsAskedToQuit())
    {
        // We will receive response synchronously, safe to put variables
        // on stack.
        brpc_benchmark::BenchmarkMessage request;
        brpc_benchmark::BenchmarkMessage response;
        brpc::Controller cntl;

        request.CopyFrom(prepare_args());

        cntl.set_log_id(log_id++); // set by user

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        stub.Say(&cntl, &request, &response, NULL);
        if (!cntl.Failed())
        {
            LOG(INFO) << "Received response from " << cntl.remote_side()
                      << " to " << cntl.local_side()
                      << ": " << response.field1() << " (attached="
                      << cntl.response_attachment() << ")"
                      << " latency=" << cntl.latency_us() << "us";
        }
        else
        {
            LOG(WARNING) << cntl.ErrorText();
        }
        usleep(interval_ms * 1000L);
    }

    LOG(INFO) << "EchoClient is going to quit";
    return 0;
}