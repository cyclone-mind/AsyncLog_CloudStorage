#pragma once
#include "./backlog_code/SendBackupLog.hpp"
#include "AsyncBuffer.hpp"
#include "AsyncWorker.hpp"
#include "Level.hpp"
#include "LogFlush.hpp"
#include "Message.hpp"
#include "ThreadPool.hpp"
#include <cstdarg>
#ifndef ASYNCLOG_CLOUDSTORAGE_ASYNCLOGGER_HPP
#define ASYNCLOG_CLOUDSTORAGE_ASYNCLOGGER_HPP
extern ThreadPool *tp; // 若在头文件表示"这个变量存在，但不在这里定义"
namespace mylog {
class AsyncLogger {
  public:
    using ptr = std::shared_ptr<AsyncLogger>;
    AsyncLogger(std::string name, std::vector<LogFlush::ptr> flushes,
                AsyncType type)
        : logger_name_(std::move(name)), flushes_(std::move(flushes)),
          asyncworker(std::make_shared<AsyncWorker>( // 启动异步工作器
              std::bind(&AsyncLogger::RealFlush, this, std::placeholders::_1),
              type)) {}
    [[nodiscard]] std::string Name() const { return logger_name_; }
    void Debug(const std::string &file, size_t line, const std::string format,
               ...) {
        va_list args;
        va_start(args, format);
        char *ret;
        if (vasprintf(&ret, format.c_str(), args) < 0) {
            perror("vasprintf failed::");
        }
        va_end(args);
        serialize(LogLevel::value::DEBUG, file, line, ret);
        free(ret);
        ret = nullptr;
    }
    void Info(const std::string &file, const size_t line,
              const std::string format, ...) {
        va_list va;
        va_start(va, format);
        char *ret;
        if (vasprintf(&ret, format.c_str(), va) == -1)
            perror("vasprintf failed!!!: ");
        va_end(va);

        serialize(LogLevel::value::INFO, file, line, ret);

        free(ret);
        ret = nullptr;
    };
    void Warn(const std::string &file, const size_t line,
              const std::string format, ...) {
        va_list va;
        va_start(va, format);
        char *ret;
        if (vasprintf(&ret, format.c_str(), va) == -1)
            perror("vasprintf failed!!!: ");
        va_end(va);

        serialize(LogLevel::value::WARN, file, line, ret);

        free(ret);
        ret = nullptr;
    };
    void Error(const std::string &file, const size_t line,
               const std::string format, ...) {
        va_list va;
        va_start(va, format);
        char *ret;
        if (vasprintf(&ret, format.c_str(), va) == -1)
            perror("vasprintf failed!!!: ");
        va_end(va);

        serialize(LogLevel::value::ERROR, file, line, ret);

        free(ret);
        ret = nullptr;
    };
    void Fatal(const std::string &file, const size_t line,
               const std::string format, ...) {
        va_list va;
        va_start(va, format);
        char *ret;
        if (vasprintf(&ret, format.c_str(), va) == -1)
            perror("vasprintf failed!!!: ");
        va_end(va);

        serialize(LogLevel::value::FATAL, file, line, ret);

        free(ret);
        ret = nullptr;
    };

  protected:
    void serialize(LogLevel::value level, const std::string &file, size_t line,
                   char *log) {
        LogMessage logmessage(level, file, line, log, logger_name_);
        std::string data = logmessage.format();
        if (level == LogLevel::value::ERROR || level == LogLevel::value::FATAL) {
            try
            {
                auto ret = tp->addTask(send_backlog, data);
                ret.get();
            }
            catch (const std::runtime_error &e)
            {
                // 该线程池没有把stop设置为true的逻辑，所以不做处理
                std::cout << __FILE__ << __LINE__ << "thread pool closed" << std::endl;
            }
        }
        Flush(data.c_str(), data.size());
    }
    void Flush(const char *data, size_t len) { asyncworker->Push(data, len); }
    void RealFlush(Buffer &buffer) {
        if (flushes_.empty()) {
            return;
        }
        for (const auto &flush : flushes_) {
            flush->Flush(buffer.Begin(), buffer.ReadableSize());
        }
    }

  protected:
    std::string logger_name_;
    std::vector<LogFlush::ptr> flushes_; // 输出到指定方向\
    // std::vector<LogFlush> flushes_;不能使用 Logflush 作为元素类型，Logflush 是纯虚类，不能实例化
    AsyncWorker::ptr asyncworker;
};

class LoggerBuilder {
  public:
    using ptr = std::shared_ptr<LoggerBuilder>;
    void BuildName(const std::string &name) { logger_name_ = name; }
    void BuildLoggerType(const AsyncType type) { async_type_ = type; }

    template <typename FlushType, typename... Args>
    void BuildLoggerFlush(Args &&...args) {
        flushes_.emplace_back(LogFlushFactory::CreateLogFlush<FlushType>(
            std::forward<Args>(args)...));
    }
    AsyncLogger::ptr Build() {
        assert(!logger_name_.empty());
        if (flushes_.empty()) {
            flushes_.emplace_back(std::make_shared<StdoutFlush>());
        }
        return std::make_shared<AsyncLogger>(logger_name_, flushes_,
                                             async_type_);
    }

  protected:
    std::string logger_name_{"default"};           // 日志器名称
    std::vector<LogFlush::ptr> flushes_;           // 刷盘方式
    AsyncType async_type_ = AsyncType::ASYNC_SAFE; // 缓冲区增长模式
};
} // namespace mylog

#endif // ASYNCLOG_CLOUDSTORAGE_ASYNCLOGGER_HPP

/*
  *  `vasprintf` 的特点：
   1. 分配的内存是以 null 结尾的字符串（'\0' 结尾）
   2. 返回的 char* 指向一个完整的 C 风格字符串
   3. std::string 的构造函数接受 const char* 参数时，会自动查找 null
  终止符来确定字符串长度


 */
