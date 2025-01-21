#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

// 任务的抽象基类
class Task {
public:
    // 用户可以自定义任务类型，从Task类继承而来，重写run()方法 
    virtual void run() = 0;
private:

};

// 线程池的类型
enum class PoolMode {
    Mode_Fixed,
    Mode_Cached,
};

// Class Thread
class Thread {
public:

private:

};

// Class ThreadPool
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    // 开启线程池
    void start(); 

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

    std::vector<Thread*> threads_;  // 线程列表
    size_t initThreadSize_;  // 初始线程数量

    std::queue<std::shared_ptr<Task>> taskQue_;  // 任务队列
    std::atomic_uint taskSize_;  // 任务数量
    size_t taskQueThreshold_;  // 任务队列容量上限

    std::mutex taskQueMutex_;  // 保证任务队列的线程安全
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
};

#endif