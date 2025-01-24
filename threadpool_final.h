#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <future>

const size_t TASK_QUE_THRESHOLD = 2;
const size_t THREAD_SIZE_THRESHOLD = 100;
const int THREAD_MAX_IDEL_TIME = 10;


// 线程类型
class Thread {
public:
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc func) 
        : func_(func),
          threadId_(genId_++) {
        }

    ~Thread() = default;

    // 启动线程
    void start() {
        std::thread t(func_, threadId_);
        t.detach();
    }

    // 查询线程id
    int getId() const {
        return threadId_;
    }

private:
    ThreadFunc func_;
    static int genId_;
    int threadId_;
};

int Thread::genId_ = 0;

// 线程池工作模式
enum class PoolMode {
    Mode_Fixed,
    Mode_Cached,
};


// 线程池类型
class ThreadPool {
public:
    ThreadPool()   
    : initThreadSize_(0), 
      idleThreadSize_(0),
      curThreadSize_(0),
      threadSizeThreshold_(THREAD_SIZE_THRESHOLD),
      taskSize_(0), 
      taskQueThreshold_(TASK_QUE_THRESHOLD), 
      poolMode_(PoolMode::Mode_Fixed),
      isRunning_(false) {
    }

    ~ThreadPool() {
        isRunning_ = false;
        std::unique_lock<std::mutex> lock(taskQueMutex_);
        notEmpty_.notify_all();
        exitCond_.wait(lock, [&]()->bool { return threads_.size() == 0; });
    }

    // 开启线程池
    void start(size_t initThreadSize = std::thread::hardware_concurrency()) {
        // 设置线程池运行状态
        isRunning_ = true;

        // 初始化线程数量
        initThreadSize_ = initThreadSize;
        curThreadSize_ = initThreadSize;

        // 创建线程对象
        for (int i = 0; i < initThreadSize_; i++) {
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFuc, this, std::placeholders::_1));
            int thread_id = ptr->getId();
            threads_.emplace(thread_id, std::move(ptr));
        }
        // 启动所有线程
        for (int i = 0; i < initThreadSize_; i++) {
            threads_[i]->start();
            idleThreadSize_++;
        }
    }

    // 设置线程池的工作模式
    void setMode(PoolMode mode) {
        if (checkState()) {
            return ;
        } 
        poolMode_ = mode; 
    }

    // 设置线程数量上限(Cached模式下)
    void setThreadSizeThreshold(size_t thread_Threshold) {
        if (checkState()) {
            return ;
        }
        if (poolMode_ == PoolMode::Mode_Cached) {
            threadSizeThreshold_ = thread_Threshold;
        }
    }

    // 设置线程池的任务队列容量上限
    void setTaskQueThreshold(size_t task_Threshold) {
        taskQueThreshold_ = task_Threshold;
    }

    // 提交任务至线程池
    template <typename Func, typename... Args>
    auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
        using RType = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<RType()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<RType> result = task->get_future();

        // 获取锁
        std::unique_lock<std::mutex> lock(taskQueMutex_);

        // 线程通信 等待任务队列空余
        if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool { return taskQue_.size() < taskQueThreshold_; })) {
            std::cerr << "Task queue is full, submit task fail." << std::endl;
            auto task = std::make_shared<std::packaged_task<RType()>>(
                []()->RType { return RType(); });
            (*task)();
            return task->get_future();
        }

        // 任务队列空余，将任务加入队列
        // taskQue_.emplace(sp);
        taskQue_.emplace([task]() { (*task)(); });
        taskSize_++;

        notEmpty_.notify_all(); 

        // Cached模式下，根据任务数量和空闲线程数量判断是否需要创建新线程
        if (poolMode_ == PoolMode::Mode_Cached && taskSize_ > idleThreadSize_ && curThreadSize_ < threadSizeThreshold_) {
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFuc, this, std::placeholders::_1));
            std::cout << "---Create new thread---" << std::endl;
            int thread_id = ptr->getId();
            threads_.emplace(thread_id, std::move(ptr));
            threads_[thread_id]->start();
            curThreadSize_++;
            idleThreadSize_++;
        }

        return result;
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    PoolMode poolMode_;  // 线程池工作模式 
    std::atomic_bool isRunning_;  // 线程池是否已经启动

    std::unordered_map<int, std::unique_ptr<Thread>> threads_;  // 线程列表
    size_t initThreadSize_;  // 初始线程数量
    size_t threadSizeThreshold_;  // 线程数量上限(Cached模式下)
    std::atomic_int idleThreadSize_;  // 空闲线程数量
    std::atomic_int curThreadSize_;  // 当前线程数量

    using Task = std::function<void()>;
    std::queue<Task> taskQue_;  // 任务队列
    std::atomic_uint taskSize_;  // 任务数量
    size_t taskQueThreshold_;  // 任务队列容量上限

    std::mutex taskQueMutex_;  // 保证任务队列的线程安全
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::condition_variable exitCond_;  // 等带线程资源全部回收 

    void threadFuc(int thread_id) {
        auto lastTime = std::chrono::high_resolution_clock().now();

        for (;;) {
            Task task;
            // 建立作用域限制线程获取锁的时间
            {
                // 获取锁
                std::unique_lock<std::mutex> lock(taskQueMutex_);

                std::cout << "Thread id: " << std::this_thread::get_id() << " try to acquire task..." << std::endl;

                // Cached模式下，回收超过空闲时间的多余线程
                while (taskQue_.size() == 0) {
                    if (!isRunning_) {
                        threads_.erase(thread_id);
                        std::cout << "Thread id: " << std::this_thread::get_id() << " exit..." << std::endl;
                        exitCond_.notify_all();
                        return ;
                    }

                    if (poolMode_ == PoolMode::Mode_Cached) {
                        if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
                            auto nowTime = std::chrono::high_resolution_clock().now();
                            auto durTime = std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastTime);
                            if (durTime.count() >= THREAD_MAX_IDEL_TIME && curThreadSize_ > initThreadSize_) {
                                threads_.erase(thread_id);
                                std::cout << "---Thread exit, id: " << std::this_thread::get_id() << "---" << std::endl; 
                                curThreadSize_--;
                                idleThreadSize_--;
                                return ;
                            }
                        }
                    } else {
                        // 等待notEmpty条件
                        notEmpty_.wait(lock);
                    }
                }

                std::cout << "Thread id: " << std::this_thread::get_id() << " acquire task successfully..." << std::endl;
                idleThreadSize_--;

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
                task();
            }

            idleThreadSize_++;
            lastTime = std::chrono::high_resolution_clock().now();
        }
    }  // 线程执行函数
    bool checkState() const{
        return isRunning_;
    }  // 查询线程池运行状态
};

#endif