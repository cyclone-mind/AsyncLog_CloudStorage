
#ifndef ASYNCLOG_CLOUDSTORAGE_ASYNCWORKER_HPP
#define ASYNCLOG_CLOUDSTORAGE_ASYNCWORKER_HPP
#include "AsyncBuffer.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace mylog {
enum class AsyncType { ASYNC_SAFE, ASYNC_UNSAFE };
class AsyncWorker {
  public:
    using ptr = std::shared_ptr<AsyncWorker>;
    AsyncWorker(const std::function<void(Buffer &)> &cb,
                AsyncType async_type = AsyncType::ASYNC_SAFE)
        : async_type_(async_type), stop_(false),
          thread_(&AsyncWorker::ThreadEntry, this), callback_(cb) {}
    ~AsyncWorker() { Stop(); }
    void Stop() {
        stop_ = true;
        cond_consumer_.notify_all();
        thread_.join();
    }
    void Push(const char *data, const size_t len) {

        std::unique_lock<std::mutex> lock(mutex_);
        // 如果缓冲区是固定大小，则等待生产者写入数据
        if (AsyncType::ASYNC_SAFE == async_type_) {
            cond_productor_.wait(lock, [&]() {
                return len <= buffer_productor_.WriteableSize() || stop_;
            });
            if (stop_)
                return;
        }
        // 写入数据
        buffer_productor_.Push(data, len);
        // 通知消费者读取数据
        lock.unlock();
        cond_consumer_.notify_one();
    }

  private:
    void ThreadEntry() {
        while (!stop_) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_consumer_.wait(lock, [&]() {
                    return !buffer_consumer_.IsEmpty() || stop_;
                });
                buffer_productor_.Swap(buffer_consumer_);
                if (AsyncType::ASYNC_UNSAFE == async_type_) {
                    cond_productor_.notify_one();
                }
            }
            callback_(buffer_consumer_);
            buffer_consumer_.Reset();
            if (stop_ && buffer_productor_.IsEmpty())
                return;
        }
    }

  private:
    Buffer buffer_productor_;
    Buffer buffer_consumer_;
    std::condition_variable cond_productor_;
    std::condition_variable cond_consumer_;
    std::mutex mutex_;
    AsyncType async_type_;
    std::atomic<bool> stop_; // 控制异步工作器的启动
    std::thread thread_;
    std::function<void(Buffer &)> callback_;
};
} // namespace mylog

#endif // ASYNCLOG_CLOUDSTORAGE_ASYNCWORKER_HPP
