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

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

using namespace std;

mutex mtx;

int64_t get_current_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL); //该函数在sys/time.h头文件中
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

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

    string port = ":9092";              // "IP Address of server");
    string load_balancer = "";                     // "The algorithm for load balancing");
    int32_t timeout_ms = 100;                      // "RPC timeout in milliseconds");
    int32_t max_retry = 3;                         // "Max retries(not including the first RPC)");
    string http_content_type = "application/json"; // "Content type of http request");

    string protocol = "baidu_std"; // Protocol type. Defined in src/brpc/options.proto
    string connection_type = "";   // Connection type. Available values: single, pooled, short

    long threads_num = 1;
    long requests_num = 1;
    string host;
    if (argc == 4)
    {
        threads_num = atol(argv[1]);
        requests_num = atol(argv[2]);
        host = argv[3];
    }

    long per_client_num = requests_num / threads_num;

    // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::Channel channel;

    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = protocol;
    options.connection_type = connection_type;
    options.timeout_ms = timeout_ms /*milliseconds*/;
    options.max_retry = max_retry;

    if (channel.Init((host+port).c_str(), load_balancer.c_str(), &options) != 0)
    {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    // brpc_benchmark::Hello_Stub stub(&channel);

    // Send a request and wait for the response every 1 second.

    atomic<int> trans(0);
    atomic<long> trans_ok(0);

    vector<uint64_t> stats;
    cout << "begin" << endl;
    int64_t start_time = get_current_time();

    for (long i = 0; i < threads_num; i++)
    {
        thread t([per_client_num, &channel, &trans, &trans_ok, &stats]() {
            brpc_benchmark::Hello_Stub stub(&channel);
            brpc_benchmark::BenchmarkMessage request;
            brpc_benchmark::BenchmarkMessage response;
            
            vector<uint64_t> stat;

            request.CopyFrom(prepare_args());
            for (long j = 0; j < per_client_num; j++)
            {
                brpc::Controller cntl;
                uint64_t start = get_current_time();
                stub.Say(&cntl, &request, &response, NULL);
                uint64_t cost = get_current_time() - start;
                
                stat.push_back(cost);
                if (!cntl.Failed() && response.field1().compare("OK") == 0 &&
                    response.field2() == 100)
                {
                    trans_ok++;
                }
            }
            mtx.lock();
            stats.insert(stats.end(), stat.begin(), stat.end());
            mtx.unlock();
            trans++;
        });
        t.detach();
        // t.join();
    }

     while (trans.load() < threads_num)
    {
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    auto cost_time = (get_current_time() - start_time) / 1000.0;
    cout << "time cost(s): " << cost_time << endl;
    while (stats.size() < (size_t)requests_num)
    {
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    sort(stats.begin(), stats.end());
    
    cout << "sent     requests    : " << requests_num << endl;
    cout << "received requests_OK : " << trans_ok << endl;
    cout << "mean(ms):   "
         << accumulate(stats.begin(), stats.end(), 0.0) / stats.size() << endl;
    cout << "median(ms): " << stats[stats.size() / 2] << endl;
    cout << "max(ms):    " << *(stats.end() - 1) << endl;
    cout << "min(ms):    " << *(stats.begin()) << endl;
    cout << "99P(ms):    " << stats[int(stats.size() * 0.999)] << endl;
    if (cost_time > 0)
        cout << "throughput (TPS): " << (double)requests_num / cost_time << endl;
    return 0;
}
