#include "threadpool.h"

const size_t TASK_QUE_THRESHOLD = 1024;

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
    // 
    initThreadSize_ = initThreadSize;
    // 创建线程对象
    for (int i = 0; i < initThreadSize_; i++) {
        threads_.emplace_back(new Thread());
    }

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

// 提交任务至线程池
void ThreadPool::submitTask(std::shared_ptr<Task> sp) {

}

