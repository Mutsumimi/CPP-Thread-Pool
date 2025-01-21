#include "threadpool.h"

const size_t TASK_QUE_THRESHOLD = 1024;

ThreadPool::ThreadPool()   
    : initThreadSize_(4)
    , taskSize_(0)
    , taskQueThreshold_(TASK_QUE_THRESHOLD)
    , poolMode_(PoolMode::Mode_Fixed)
{}

ThreadPool::~ThreadPool()
{}

void ThreadPool::setMode(PoolMode mode) {
    poolMode_ = mode; 
}

