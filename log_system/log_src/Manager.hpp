
#ifndef ASYNCLOG_CLOUDSTORAGE_MANAGER_HPP
#define ASYNCLOG_CLOUDSTORAGE_MANAGER_HPP
#include "AsyncLogger.hpp"
namespace mylog {
class LoggerManager {
  public:
    static LoggerManager &GetInstance() {
        static LoggerManager instance;
        return instance;
    }
    bool LoggerExist(const std::string &name) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = loggers_.find(name);
        if (it == loggers_.end())
            return false;
        return true;
    }

    void AddLogger(const AsyncLogger::ptr &&AsyncLogger) {
        if (LoggerExist(AsyncLogger->Name()))
            return;
        std::unique_lock<std::mutex> lock(mutex_);
        loggers_.insert(std::make_pair(AsyncLogger->Name(), AsyncLogger));
    }

    AsyncLogger::ptr GetLogger(const std::string &name) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = loggers_.find(name);
        if (it == loggers_.end())
            return AsyncLogger::ptr();
        return it->second;
    }

    AsyncLogger::ptr DefaultLogger() { return default_logger_; }

  private:
    LoggerManager() {
        auto builder = std::make_unique<LoggerBuilder>();
        builder->BuildName("default");
        default_logger_ = builder->Build();
        loggers_.insert(std::make_pair("default", default_logger_));
    }

  private:
    AsyncLogger::ptr default_logger_;
    std::mutex mutex_;
    std::map<std::string, AsyncLogger::ptr> loggers_;
};
} // namespace mylog
#endif // ASYNCLOG_CLOUDSTORAGE_MANAGER_HPP
