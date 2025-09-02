#pragma once
#include "Util.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

namespace mylog {
class Buffer {
  public:
    Buffer() : write_pos_(0), read_pos_(0) {
        buffer_.resize(util::LogConfig::GetJsonData()->buffer_size);
    }

    /**
     * @brief 向缓冲区写入数据
     * @param data 指向待写入数据的指针
     * @param len 待写入数据的长度
     */
    void Push(const char *data, const size_t len) {
        ToBeEnough(len);
        std::copy(data, data + len, buffer_.data() + write_pos_);
        write_pos_ += len;
    }

    /**
     * @brief 检查缓冲区是否为空
     * @note 当读写位置相等时认为缓冲区为空
     */
    [[nodiscard]] bool IsEmpty() const { return write_pos_ == read_pos_; }

    /**
     * @brief 获取读取起始位置的指针
     * @param len 欲读取的长度
     * @return char* 指向读取起始位置的指针
     * @note 调用前需确保有足够的可读数据，通过ReadableSize()检查
     */
    char *ReadBegin(const int len) {
        assert(len <= ReadableSize());
        return &buffer_[read_pos_];
    }

    /**
     * @brief 与另一个Buffer对象交换内容
     * @param buf 要交换的Buffer对象引用
     * @note 通过交换内部数据指针和位置变量实现高效交换
     */
    void Swap(Buffer &buf) {
        buffer_.swap(buf.buffer_);
        std::swap(read_pos_, buf.read_pos_);
        std::swap(write_pos_, buf.write_pos_);
    }

    /**
     * @brief 获取可读数据的起始位置
     * @return const char* 指向可读数据起始位置的常量指针
     */
    [[nodiscard]] const char *Begin() const { return &buffer_[read_pos_]; }


    [[nodiscard]] size_t WriteableSize() const { // 写空间剩余容量
        return buffer_.size() - write_pos_;
    }

    [[nodiscard]] size_t ReadableSize() const { // 读空间剩余容量
        return write_pos_ - read_pos_;
    }

    void Reset()
    { // 重置缓冲区
        write_pos_ = 0;
        read_pos_ = 0;
    }

  protected:
    /**
     * @brief 确保缓冲区有足够的空间容纳指定长度的数据
     * @param len 需要确保存储空间的字节长度
     */
    void ToBeEnough(const size_t len) {
        const auto buffersize = buffer_.size();
        if (len >= WriteableSize()) {
            // 根据当前缓冲区大小和配置阈值选择不同的扩容策略
            if (buffer_.size() < util::LogConfig::GetJsonData()->threshold) {
                // 当前大小 小于 阈值时，采用指数扩容：容量翻倍
                buffer_.resize(1 * buffer_.size() + buffersize);
            } else {
                // 当前大小 大于 等于阈值时，采用线性扩容：增加固定大小
                buffer_.resize(util::LogConfig::GetJsonData()->linear_growth +
                               buffersize);
            }
        }
    }

    std::vector<char> buffer_; // 缓冲区
    size_t write_pos_; // 写位置
    size_t read_pos_; //
};
} // namespace mylog

/*
 *
 * copy 时 这里 write_pos_ 是 size_t 类型，
 * 而 buffer_.begin() 返回的迭代器在进行算术运算时可能需要转换为
 * difference_type（有符号类型）
 */