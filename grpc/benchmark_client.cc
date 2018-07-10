
#include "benchmark.grpc.pb.h"

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <grpc/grpc.h>

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

grpc_benchmark::BenchmarkMessage prepare_args()
{
    grpc_benchmark::BenchmarkMessage msg;
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

class HelloClient
{
  public:
    HelloClient(std::shared_ptr<grpc::Channel> channel)
        : stub(grpc_benchmark::Hello::NewStub(channel)) {}

    bool say(grpc_benchmark::BenchmarkMessage msg)
    {
        grpc::ClientContext context;
        grpc_benchmark::BenchmarkMessage response;
        grpc::Status OK = stub->Say(&context, msg, &response);
        if (OK.ok() && response.field1().compare("OK") == 0 &&
            response.field2() == 100)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    long run_client(long n)
    {
        long k = 0;
        for (long i = 0; i < n; i++)
        {
            grpc_benchmark::BenchmarkMessage msg = prepare_args();
            uint64_t start = get_current_time();
            bool ok = say(msg);
            uint64_t cost = get_current_time() - start;
            costs.push_back(cost);
            if (ok)
            {
                k++;
            }
        }
        return k;
    }
    vector<uint64_t> costs;

  private:
    unique_ptr<grpc_benchmark::Hello::Stub> stub;
};

int main(int argc, char *argv[])
{
    cout << "startup" << endl;
    if (argc <= 1)
    {
        cout << "no parms" << endl;
        return -1;
    }
    int threads_num = atoi(argv[1]);
    int requests_num = atoi(argv[2]);

    grpc_benchmark::BenchmarkMessage msg = prepare_args();

    const string addr = "127.0.0.1:9091";
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());

    int per_client_num = requests_num / threads_num;

    atomic<int> trans(0);
    atomic<long> trans_ok(0);

    vector<uint64_t> stats;
    cout << "begin" << endl;
    int64_t start_time = get_current_time();

    for (int i = 0; i < threads_num; i++)
    {
        thread t([per_client_num, &channel, &trans, &trans_ok, &stats]() {
            HelloClient client(channel);
            trans_ok.fetch_add(client.run_client(per_client_num));
            trans++;
            mtx.lock();
            stats.insert(stats.end(), client.costs.begin(), client.costs.end());
            mtx.unlock();
        });
        t.detach();
        // t.join();
    }

    while (trans.load() < threads_num)
    {
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    auto cost_time = (get_current_time() - start_time) / 1000;
    cout << "time cost(s): " << cost_time << endl;
    while (stats.size() < (size_t)requests_num)
    {
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    // TODO:
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
        cout << "throughput (TPS): " << requests_num / cost_time << endl;
    return 0;
}