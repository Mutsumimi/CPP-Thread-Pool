#include <iostream>
#include <chrono>
#include <thread>

#include "threadpool.h"

using ULONG = unsigned long long;

class MyTask : public Task {
public:
    MyTask(int v1, int v2) : begin_(v1), end_(v2) {}
    Any run() {
        std::cout << "Thread id: " << std::this_thread::get_id() << " begin." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        ULONG sum = 0;
        for (ULONG i = begin_; i <= end_; i++) {
            sum += i;
        }
        std::cout << "Thread id: " << std::this_thread::get_id() << " end." << std::endl;
        return sum;
    }
private:
    int begin_;
    int end_;
};

int main() {
    {
        ThreadPool pool;
        pool.setMode(PoolMode::Mode_Cached);
        pool.start(4);

        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        Result res4 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        // pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        // Result res6 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

        ULONG sum1 = res1.get().cast_<ULONG>();
        ULONG sum2 = res2.get().cast_<ULONG>();
        ULONG sum3 = res3.get().cast_<ULONG>();
        ULONG sum4 = res4.get().cast_<ULONG>();
        std::cout << sum1 + sum2 + sum3 << std::endl;
    }

    std::cout << "main over" << std::endl;
    getchar();
}