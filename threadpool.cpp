#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>

const size_t TASK_QUE_THRESHOLD = 1024;

/*
线程池类方法实现
*/

ThreadPool::ThreadPool()   
    : initThreadSize_(0)
    , taskSize_(0)
    , taskQueThreshold_(TASK_QUE_THRESHOLD)
    , poolMode_(PoolMode::Mode_Fixed)
{}

ThreadPool::~ThreadPool()
{}

// 开启线程池
void ThreadPool::start(size_t initThreadSize) {
    // 初始化线程数量
    initThreadSize_ = initThreadSize;
    // 创建线程对象
    for (int i = 0; i < initThreadSize_; i++) {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFuc, this));
        threads_.emplace_back(std::move(ptr));
    }
    // 启动所有线程
    for (int i = 0; i < initThreadSize_; i++) {
        threads_[i]->start();
    }
}

// 设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode) {
    poolMode_ = mode; 
}

// 设置线程池的任务队列容量上限
void ThreadPool::setTaskQueThreshold(size_t threshold) {
    taskQueThreshold_ = threshold;
}

// 提交任务至线程池    用户调用该接口向任务队列中添加任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp) {
    // 获取锁
    std::unique_lock<std::mutex> lock(taskQueMutex_);

    // 线程通信 等待任务队列空余
    if (notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool { return taskQue_.size() < taskQueThreshold_; })) {
        std::cerr << "Task queue is full, submit task fail." << std::endl;
        return ;
    }

    // 任务队列空余，将任务加入队列
    taskQue_.emplace(sp);
    taskSize_++;

    notEmpty_.notify_all(); 
}

// 定义线程函数    线程池的所有线程从任务队列中获取任务并执行
void ThreadPool::threadFuc() {
    for (;;) {
        std::shared_ptr<Task> task;
        // 建立作用域限制线程获取锁的时间
        {
            // 获取锁
            std::unique_lock<std::mutex> lock(taskQueMutex_);

            // 等待notEmpty条件
            notEmpty_.wait(lock, [&]()->bool { return taskQue_.size() > 0; });

            // 从任务队列中获取任务
            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;

            if (taskSize_ > 0) {
                notEmpty_.notify_all();
            }

            notFull_.notify_all();
        }

        // 当前线程执行该任务
        if (task != nullptr) {
            task->run();
        }
    }
}

/*
线程类方法实现
*/

Thread::Thread(ThreadFunc func) 
    : func_(func)
{}

Thread::~Thread()
{}

// 启动线程
void Thread::start() {
    std::thread t(func_);
    t.detach();
}
