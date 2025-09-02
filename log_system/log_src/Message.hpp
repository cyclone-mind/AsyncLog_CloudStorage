#pragma once
#include "Level.hpp"
#include "Util.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#ifndef ASYNCLOG_CLOUDSTORAGE_MESSAGE_HPP
#define ASYNCLOG_CLOUDSTORAGE_MESSAGE_HPP

namespace mylog {
struct LogMessage {
    using ptr = std::shared_ptr<LogMessage>;
    LogMessage(const LogLevel::value level, std::string file, const size_t line,
               std::string message, std::string name)
        : level_(level), filename_(std::move(file)), line_(line),
          timestamp_(util::Date::Now()), message_(std::move(message)),
          logger_name_(std::move(name)), tid_(std::this_thread::get_id()) {}

    [[nodiscard]] std::string format() const {
        std::stringstream ret;
        tm t{};
        localtime_r(&timestamp_, &t);
        char buf[128];
        strftime(buf, sizeof(buf), "%H:%M:%S", &t);
        const std::string tmp1 = '[' + std::string(buf) + "][";
        const std::string tmp2 = "][" + std::string(LogLevel::ToString(level_)) +
                                 "][" + logger_name_ + "][" + filename_ + ":" +
                                 std::to_string(line_) + "]\t" + message_ +
                                 "\n";
        ret << tmp1 << tid_ << tmp2;
        return ret.str();
    }

    LogLevel::value level_{};
    std::string filename_;
    size_t line_{};
    time_t timestamp_{};
    std::string message_;
    std::string logger_name_;
    std::thread::id tid_; // 线程id
};
} // namespace mylog

#endif // ASYNCLOG_CLOUDSTORAGE_MESSAGE_HPP

/*
 * 建议任何时候都要考虑类成员有无默认初始化
 * size_t line_; 写法没有初始化，值未定义，建议 size_t line_{};
 * std::string 不需要 {} 初始化是因为它自动调用默认构造函数，初始化为空字符串 ""
 * LogLevel::value
 * level_;这样写没什么问题，但是编辑器可能提示未初始化值，建议也写为LogLevel::value
 * level_{};
 *
 * 不依赖任何其他库，包括 chrono， 得到当前时间字符串
 * 要得到字符串格式的时间，可以先 time(nullptr)得到time_t类型，然后再构建一个tm的struct t。
 * 随后使用localtime_r(&timestamp_, &t);将时间转换为tm，再使用方法 strftime(buf, sizeof(buf), "%H:%M:%S", &t);
 *
 * [[nodiscard]] 是一个 C++17  特性，它告诉编译器：这个函数的返回值很重要，不应该被忽略。
 * 如果调用代码这样写：msg.format(); // 返回值被忽略了
  编译器会给出警告，提醒你忘记使用返回值。
 */
