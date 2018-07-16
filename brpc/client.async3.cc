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

// A client sending requests to server in parallel by multiple threads.

#include "benchmark.pb.h"

#include <brpc/parallel_channel.h>
#include <brpc/server.h>
#include <bthread/bthread.h>
#include <butil/logging.h>
#include <butil/macros.h>
#include <butil/string_printf.h>
#include <butil/time.h>
#include <gflags/gflags.h>

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_int32(channel_num, 3, "Number of sub channels");
DEFINE_bool(same_channel, false, "Add the same sub channel multiple times");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_int32(attachment_size, 0,
             "Carry so many byte attachment along with requests");
DEFINE_int32(request_size, 16, "Bytes of each request");
DEFINE_string(connection_type, "",
              "Connection type. Available values: single, pooled, short");
DEFINE_string(protocol, "baidu_std",
              "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(server, "0.0.0.0:9092", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(dummy_port, -1, "Launch dummy server at this port");

std::string g_request;
std::string g_attachment;
bvar::LatencyRecorder g_latency_recorder("client");
bvar::Adder<int> g_error_count("client_error_count");
bvar::LatencyRecorder *g_sub_channel_latency = NULL;

brpc_benchmark::BenchmarkMessage prepare_args()
{
    brpc_benchmark::BenchmarkMessage msg;
    bool b = true;
    int i = 100000;
    std::string str = "许多往事在眼前一幕一幕，变的那麼模糊";
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

    int log_id = 0;
    while (!brpc::IsAskedToQuit())
    {
        // We will receive response synchronously, safe to put variables
        // on stack.
        brpc_benchmark::BenchmarkMessage request;
        brpc_benchmark::BenchmarkMessage *response =
            new brpc_benchmark::BenchmarkMessage();
        brpc::Controller *cntl = new brpc::Controller();

        request.CopyFrom(prepare_args());

       google::protobuf::Closure *done =
            brpc::NewCallback(&HandleEchoResponse, cntl, response);
        stub.Say(cntl, &request, response, done);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::ParallelChannel channel;
    brpc::ParallelChannelOptions pchan_options;
    pchan_options.timeout_ms = FLAGS_timeout_ms;
    if (channel.Init(&pchan_options) != 0)
    {
        LOG(ERROR) << "Fail to init ParallelChannel";
        return -1;
    }

    brpc::ChannelOptions sub_options;
    sub_options.protocol = FLAGS_protocol;
    sub_options.connection_type = FLAGS_connection_type;
    sub_options.max_retry = FLAGS_max_retry;
    // Setting sub_options.timeout_ms does not work because timeout of sub
    // channels are disabled in ParallelChannel.

    if (FLAGS_same_channel)
    {
        // For brpc >= 1.0.155.31351, a sub channel can be added into
        // a ParallelChannel more than once.
        brpc::Channel *sub_channel = new brpc::Channel;
        // Initialize the channel, NULL means using default options.
        // options, see `brpc/channel.h'.
        if (sub_channel->Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(),
                              &sub_options) != 0)
        {
            LOG(ERROR) << "Fail to initialize sub_channel";
            return -1;
        }
        for (int i = 0; i < FLAGS_channel_num; ++i)
        {
            if (channel.AddChannel(sub_channel, brpc::OWNS_CHANNEL, NULL, NULL) !=
                0)
            {
                LOG(ERROR) << "Fail to AddChannel, i=" << i;
                return -1;
            }
        }
    }
    else
    {
        for (int i = 0; i < FLAGS_channel_num; ++i)
        {
            brpc::Channel *sub_channel = new brpc::Channel;
            // Initialize the channel, NULL means using default options.
            // options, see `brpc/channel.h'.
            if (sub_channel->Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(),
                                  &sub_options) != 0)
            {
                LOG(ERROR) << "Fail to initialize sub_channel[" << i << "]";
                return -1;
            }
            if (channel.AddChannel(sub_channel, brpc::OWNS_CHANNEL, NULL, NULL) !=
                0)
            {
                LOG(ERROR) << "Fail to AddChannel, i=" << i;
                return -1;
            }
        }
    }

    // Initialize bvar for sub channel
    g_sub_channel_latency = new bvar::LatencyRecorder[FLAGS_channel_num];
    for (int i = 0; i < FLAGS_channel_num; ++i)
    {
        std::string name;
        butil::string_printf(&name, "client_sub_%d", i);
        g_sub_channel_latency[i].expose(name);
    }

    if (FLAGS_attachment_size > 0)
    {
        g_attachment.resize(FLAGS_attachment_size, 'a');
    }
    if (FLAGS_request_size <= 0)
    {
        LOG(ERROR) << "Bad request_size=" << FLAGS_request_size;
        return -1;
    }
    g_request.resize(FLAGS_request_size, 'r');

    if (FLAGS_dummy_port >= 0)
    {
        brpc::StartDummyServerAt(FLAGS_dummy_port);
    }

    std::vector<bthread_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_bthread)
    {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i)
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
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i)
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
                  << " latency=" << g_latency_recorder.latency(1) << noflush;
        for (int i = 0; i < FLAGS_channel_num; ++i)
        {
            LOG(INFO) << " latency_" << i << "="
                      << g_sub_channel_latency[i].latency(1) << noflush;
        }
        LOG(INFO);
    }

    LOG(INFO) << "EchoClient is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i)
    {
        if (!FLAGS_use_bthread)
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
