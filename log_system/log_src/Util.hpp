#pragma once
#include <bits/locale_classes.h>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#ifndef ASYNCLOG_CLOUDSTORAGE_UTIL_HPP
#define ASYNCLOG_CLOUDSTORAGE_UTIL_HPP

namespace mylog::util {
namespace fs = std::filesystem;

class Date {
  public:
    static time_t Now() { return time(nullptr); };
};

class File {
  public:
    static bool Exists(const std::string &filename) {
        return fs::exists(filename);
    }
    static std::string Path(const std::string &filename) {
        const fs::path file_path(filename);
        return file_path.parent_path().string();
    }
    static int64_t FileSize(const std::string_view filename) {
        std::error_code ec;
        const auto size = std::filesystem::file_size(filename, ec);

        if (ec) {
            std::cerr << "获取文件大小失败: " << ec.message() << std::endl;
            return -1;
        }

        // 检查文件大小是否超过int64_t的最大值
        if (size >
            static_cast<uintmax_t>(std::numeric_limits<int64_t>::max())) {
            std::cerr << "文件大小超过int64_t最大值" << std::endl;
            return -1;
        }

        return static_cast<int64_t>(size);
    }
    static bool CreateDirectory(const std::string &dir_path) {
        if (dir_path.empty()) {
            std::cerr << "目录路径为空" << std::endl;
            return false;
        }

        const fs::path directory(dir_path);

        // 如果目录不存在则创建
        if (!fs::exists(directory)) {
            std::error_code ec;
            const bool result = fs::create_directories(directory, ec);
            if (ec) {
                std::cerr << "创建目录失败: " << ec.message() << std::endl;
                return false;
            }
            return result;
        }

        return true; // 目录已存在
    }
    // 读取文件内容
    static bool GetContent(std::string *content, const std::string &filename) {
        try {
            if (!std::filesystem::exists(filename)) {
                std::cerr << "文件不存在: " << filename << std::endl;
                return false;
            }

            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "无法打开文件: " << filename << std::endl;
                return false;
            }

            auto file_size = std::filesystem::file_size(filename);
            content->resize(file_size);
            file.read(&(*content)[0], file_size);

            return file.good();

        } catch (const std::exception &e) {
            std::cerr << "读取文件异常: " << e.what() << std::endl;
            return false;
        }
    }
};

class JsonUtil {
  public:
    static bool Serialize(const Json::Value &val, std::string *str) {
        // 建造者生成->建造者实例化json写对象->调用写对象中的接口进行序列化写入str
        Json::StreamWriterBuilder swb;
        std::unique_ptr<Json::StreamWriter> usw(swb.newStreamWriter());
        std::stringstream ss;
        if (usw->write(val, &ss) != 0) {
            std::cout << "serialize error" << std::endl;
            return false;
        }
        *str = ss.str();
        return true;
    }
    static bool UnSerialize(const std::string &str, Json::Value *val) {
        // 操作方法类似序列化
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> ucr(crb.newCharReader());
        std::string err;
        if (ucr->parse(str.c_str(), str.c_str() + str.size(), val, &err) ==
            false) {
            std::cout << __FILE__ << __LINE__ << "parse error" << err
                      << std::endl;
            return false;
        }
        return true;
    }
};

struct LogConfig {
    static LogConfig *GetJsonData() {
        static auto *json_data = new LogConfig;
        return json_data;
    }

  private:
    LogConfig() {
        std::string content;
        if (File::GetContent(&content,
                             "../log_system/log_src/config.conf") == false) {
            std::cout << __FILE__ << __LINE__ << "open config.conf failed"
                      << std::endl;
            perror(nullptr);
        }
        Json::Value root;
        JsonUtil::UnSerialize(content,
                              &root); // 反序列化，把内容转成 json value格式
        buffer_size = root["buffer_size"].asInt64();
        threshold = root["threshold"].asInt64();
        linear_growth = root["linear_growth"].asInt64();
        flush_log = root["flush_log"].asInt64();
        backup_addr = root["backup_addr"].asString();
        backup_port = root["backup_port"].asInt();
        thread_count = root["thread_count"].asInt();
    }

  public:
    size_t buffer_size;   // 缓冲区基础容量
    size_t threshold;     // 倍数扩容阈值
    size_t linear_growth; // 线性增长容量
    size_t
        flush_log; // 控制日志同步到磁盘的时机，默认为0,1调用fflush，2调用fsync
    std::string backup_addr;
    uint16_t backup_port;
    size_t thread_count;
};

} // namespace mylog::util

#endif // ASYNCLOG_CLOUDSTORAGE_UTIL_HPP
