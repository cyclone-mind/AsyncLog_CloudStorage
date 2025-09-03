
#ifndef ASYNCLOG_CLOUDSTORAGE_LOGFLUSH_HPP
#define ASYNCLOG_CLOUDSTORAGE_LOGFLUSH_HPP
#include "Util.hpp"
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
namespace mylog {
class LogFlush {
  public:
    using ptr = std::shared_ptr<LogFlush>;
    virtual ~LogFlush() = default;
    virtual void Flush(const char *data, size_t len) = 0;
};
class StdoutFlush final : public LogFlush {
  public:
    using ptr = std::shared_ptr<StdoutFlush>;
    void Flush(const char *data, const size_t len) override {
        std::cout.write(data, static_cast<std::streamsize>(len));
    };
};

class FileFlush final : public LogFlush {
  public:
    using ptr = std::shared_ptr<FileFlush>;
    explicit FileFlush(std::string filename) : filename_(std::move(filename)) {
        util::File::CreateDirectory(util::File::Path(filename_));
        fs_ = fopen(filename_.c_str(), "ab");
        if (!fs_) {
            std::cout << __FILE__ << __LINE__ << " open log file failed\n";
            perror(nullptr);
        }
    }
    ~FileFlush() override {
        if (fs_) {
            fflush(fs_);
            fclose(fs_);
            fs_ = nullptr;
        }
    }
    /**
     * \brief 将日志数据写入文件并根据配置进行刷新
     *
     * 该函数将指定长度的日志数据写入文件流，根据配置决定是否以及如何刷新缓冲区。
     *
     * \param data 指向要写入文件的日志数据的指针
     * \param len 要写入的数据长度
     */
    void Flush(const char *data, const size_t len) override {
        if (!fs_)
            return;
        if (fwrite(data, 1, len, fs_) != len || ferror(fs_)) {
            std::cout << __FILE__ << __LINE__ << " write log file failed\n";
            perror(nullptr);
        }
        /*
         * 根据配置决定日志刷新策略：
         * flush_log == 1: 只刷新文件缓冲区
         * flush_log == 2: 刷新文件缓冲区并同步到磁盘
         */
        if (util::LogConfig::GetJsonData()->flush_log == 1) {
            fflush(fs_);
        } else if (util::LogConfig::GetJsonData()->flush_log == 2) {
            fflush(fs_);
            fsync(fileno(fs_));
        }
    }

  private:
    std::string filename_;
    FILE *fs_ = nullptr;
};
class RollFileFlush final : public LogFlush {
  public:
    using ptr = std::shared_ptr<RollFileFlush>;
    explicit RollFileFlush(std::string filename, size_t max_size)
        : basename_(std::move(filename)), max_size_(max_size) {}
    void Flush(const char *data, const size_t len) override { 
        InitLogFile();
        fwrite(data, 1, len, fs_);
        if(ferror(fs_)){
            std::cout <<__FILE__<<__LINE__<<"write log file failed"<< std::endl;
            perror(nullptr);
        }
        cur_size_ += len;
        /*
         * 根据配置决定日志刷新策略：
         * flush_log == 1: 只刷新文件缓冲区
         * flush_log == 2: 刷新文件缓冲区并同步到磁盘
         */
        if(util::LogConfig::GetJsonData()->flush_log == 1){
            if(fflush(fs_)){
                std::cout <<__FILE__<<__LINE__<<"fflush file failed"<< std::endl;
                perror(nullptr);
            }
        }else if(util::LogConfig::GetJsonData()->flush_log == 2){
            fflush(fs_);
            fsync(fileno(fs_));
        }
    }
    ~RollFileFlush() override {
        if (fs_ != nullptr) {
            fclose(fs_);
            fs_ = nullptr;
        }
    }

  private:
    void InitLogFile() {
        if (fs_ == nullptr || cur_size_ >= max_size_) {
            if (fs_ != nullptr) {
                fclose(fs_);
                fs_ = nullptr;
            }
            std::string filename = CreateLogFileName();
            fs_ = fopen(filename.c_str(), "ab");
            if (!fs_) {
                std::cout << __FILE__ << __LINE__ << " open log file failed\n";
                perror(nullptr);
            }
            cur_size_ = 0;
        }
    }
    std::string CreateLogFileName() {
        time_t time_ = util::Date::Now();
        tm t;
        localtime_r(&time_, &t);
        std::string filename = basename_;
        filename += std::to_string(t.tm_year + 1900);
        filename += std::to_string(t.tm_mon + 1);
        filename += std::to_string(t.tm_mday);
        filename += std::to_string(t.tm_hour + 1);
        filename += std::to_string(t.tm_min + 1);
        filename += std::to_string(t.tm_sec + 1) + '-' +
                    std::to_string(cnt_++) + ".log";
        return filename;
    }

  private:
    FILE *fs_ = nullptr;
    size_t max_size_;
    size_t cur_size_ = 0;
    size_t cnt_ = 1;
    std::string basename_;
};

} // namespace mylog

class LogFlushFactory {
  public:
    using ptr = std::shared_ptr<mylog::LogFlush>;
    template <typename FlushType, typename... Args>
    static std::shared_ptr<mylog::LogFlush> CreateLogFlush(Args &&...args) {
        return std::make_shared<FlushType>(std::forward<Args>(args)...);
    }
};

#endif // ASYNCLOG_CLOUDSTORAGE_LOGFLUSH_HPP

/*
explicit 用来禁止“隐式转换”。

*/