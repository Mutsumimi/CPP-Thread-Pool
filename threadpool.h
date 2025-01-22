#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

// Any类型 可以接受任意的数据类型
class Any {
public:
    Any() = default;
    ~Any() = default;
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    template<typename T>
    Any(T data) : base_(std::make_unique<Drive<T>>(data)) {}

private:
    // 基类类型
    class Base {
    public:
        virtual ~Base() = default;
    private:

    };

    // 派生类类型
    template<typename T>
    class Drive : public Base {
    public:
        Drive(T, data) : data_(data) {}
    private:
        T data_;
    };

    std::unique_ptr<Base> base_;  // 基类指针
};


// 任务的抽象基类
class Task {
public:
    // 用户可以自定义任务类型，从Task类继承而来，重写run()方法 
    virtual Any run() = 0;
private:

};


// 线程类型
class Thread {
public:
    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc func);
    ~Thread() = default;

    // 启动线程
    void start();

private:
    ThreadFunc func_;
};


// 线程池工作模式
enum class PoolMode {
    Mode_Fixed,
    Mode_Cached,
};


// 线程池类型
class ThreadPool {
public:
    ThreadPool() = default;
    ~ThreadPool() = default;

    // 开启线程池
    void start(size_t initThreadSize = 4); 

    // 设置线程池的工作模式
    void setMode(PoolMode mode);

    // 设置线程池的任务队列容量上限
    void setTaskQueThreshold(size_t threshold);

    // 提交任务至线程池
    void submitTask(std::shared_ptr<Task> sp);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    PoolMode poolMode_;  // 线程池工作模式 

    std::vector<std::unique_ptr<Thread>> threads_;  // 线程列表
    size_t initThreadSize_;  // 初始线程数量

    std::queue<std::shared_ptr<Task>> taskQue_;  // 任务队列
    std::atomic_uint taskSize_;  // 任务数量
    size_t taskQueThreshold_;  // 任务队列容量上限

    std::mutex taskQueMutex_;  // 保证任务队列的线程安全
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;

    void threadFuc();
};

#endif