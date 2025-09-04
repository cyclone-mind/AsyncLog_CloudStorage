#pragma once
#include "../../log_system/log_src/MyLog.hpp"
#include "Config.hpp"
#include "zstd_wrapper.h"
#include "json/json.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace storage {
namespace fs = std::filesystem;

/**
 * 将数字转换为十六进制字符
 *
 * 功能：将 0-15 的数字转换为对应的十六进制字符 '0'-'9' 或 'A'-'F'
 * 实现原理：0-9 对应字符 '0'-'9'（ASCII 48-57），10-15 对应字符 'A'-'F'（ASCII
 * 65-70）
 *
 * @param x 要转换的数字（0-15）
 * @return 对应的十六进制字符
 */
static unsigned char ToHex(unsigned char x) {
  return x > 9 ? x + 55 : x + 48; // 大于9用字母A-F，小于等于9用数字0-9
}

/**
 * 将十六进制字符转换为数字
 *
 * 功能：将十六进制字符 '0'-'9'、'A'-'Z'、'a'-'z' 转换为对应的数字 0-15
 * 支持大小写字母，不区分大小写
 *
 * @param x 要转换的十六进制字符
 * @return 对应的数字（0-15）
 * @throws 如果输入不是有效的十六进制字符，会触发 assert 断言
 */
static unsigned char FromHex(unsigned char x) {
  unsigned char y;
  if (x >= 'A' && x <= 'Z')
    y = x - 'A' + 10; // 'A'-'Z' 对应 10-35，但这里只处理 10-15
  else if (x >= 'a' && x <= 'z')
    y = x - 'a' + 10; // 'a'-'z' 对应 10-35，但这里只处理 10-15
  else if (x >= '0' && x <= '9')
    y = x - '0'; // '0'-'9' 对应 0-9
  else
    assert(0); // 无效字符触发断言
  return y;
}

/**
 * URL 解码函数
 *
 * 功能：将 URL 编码的字符串解码为原始字符串
 * URL 编码规则：特殊字符被编码为 %XX 格式，其中 XX 是字符的十六进制表示
 * 例如：%20 表示空格，%2F 表示斜杠，%3D 表示等号
 *
 * 实现逻辑：
 * 1. 遍历字符串的每个字符
 * 2. 遇到 '%' 时，读取后面两个字符作为十六进制数
 * 3. 将十六进制数转换为对应的字符
 * 4. 其他字符直接复制
 *
 * @param str 要解码的 URL 编码字符串
 * @return 解码后的原始字符串
 * @throws 如果 % 后面没有足够的字符或十六进制格式错误，会触发 assert 断言
 */
static std::string UrlDecode(const std::string &str) {
  std::string strTemp = "";
  size_t length = str.length();
  for (size_t i = 0; i < length; i++) {
    // 注意：这里注释掉了对 '+' 号的处理
    // 标准 URL 编码中，'+' 号表示空格，但这里没有实现
    // if (str[i] == '+')
    //     strTemp += ' ';

    if (str[i] == '%') {
      // 确保 % 后面有足够的字符（至少2个）
      assert(i + 2 < length);

      // 读取 % 后面的两个字符并转换为十六进制数
      unsigned char high = FromHex((unsigned char)str[++i]); // 高4位
      unsigned char low = FromHex((unsigned char)str[++i]);  // 低4位

      // 组合成完整的字节值：high * 16 + low
      strTemp += high * 16 + low;
    } else {
      // 非编码字符直接复制
      strTemp += str[i];
    }
  }
  return strTemp;
}

class FileUtil {
private:
  std::string filename_;

public:
  FileUtil(const std::string &filename) : filename_(filename) {}

  ////////////////////////////////////////////
  // 文件操作
  //  获取文件大小
  /**
   * @description: 获取文件大小
   * @return {int64_t} 文件大小
   */
  int64_t FileSize() {
    struct stat s;
    auto ret = stat(filename_.c_str(), &s);
    if (ret == -1) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s, Get file size failed: %s", filename_.c_str(),
                 strerror(errno));
      return -1;
    }
    return s.st_size;
  }
  // 获取文件最近访问时间
  /**
   * @description: 获取文件最近访问时间
   * @return {time_t} 文件最近访问时间
   */
  time_t LastAccessTime() {
    struct stat s;
    auto ret = stat(filename_.c_str(), &s);
    if (ret == -1) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s, Get file access time failed: %s", filename_.c_str(),
                 strerror(errno));
      return -1;
    }
    return s.st_atime;
  }

  // 获取文件最近修改时间
  /**
   * @description: 获取文件最近修改时间
   * @return {time_t} 文件最近修改时间
   */
  time_t LastModifyTime() {
    struct stat s;
    auto ret = stat(filename_.c_str(), &s);
    if (ret == -1) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s, Get file modify time failed: %s", filename_.c_str(),
                 strerror(errno));
      return -1;
    }
    return s.st_mtime;
  }

  // 从路径中解析出文件名
  /**
   * @description: 从路径中解析出文件名
   * @return {std::string} 文件名
   */
  std::string FileName() {
    auto pos = filename_.find_last_of("/");
    if (pos == std::string::npos)
      return filename_;
    return filename_.substr(pos + 1, std::string::npos);
  }

  // 从文件POS处获取len长度字符给content
  /**
   * @description: 从文件POS处获取len长度字符给content
   * @param {std::string*} content
   * @param {size_t} pos
   * @param {size_t} len
   * @return {bool} 是否成功
   */
  bool GetPosLen(std::string *content, size_t pos, size_t len) {
    // 判断要求数据内容是否符合文件大小
    if (pos + len > FileSize()) {
      mylog::GetLogger("cloud_storage")
          ->Info("needed data larger than file size");
      return false;
    }

    // 打开文件
    std::ifstream ifs;
    ifs.open(filename_.c_str(), std::ios::binary);
    if (ifs.is_open() == false) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s,file open error", filename_.c_str());
      return false;
    }

    // 读入content
    ifs.seekg(pos, std::ios::beg); // 更改文件指针的偏移量
    content->resize(len);
    ifs.read(&(*content)[0], len);
    if (!ifs.good()) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s,read file content error", filename_.c_str());
      ifs.close();
      return false;
    }
    ifs.close();

    return true;
  }

  // 获取文件内容
  /**
   * @description: 获取文件内容
   * @param {std::string*} content
   * @return {bool} 是否成功
   */
  bool GetContent(std::string *content) {
    return GetPosLen(content, 0, FileSize());
  }

  // 写文件
  /**
   * @description: 写文件
   * @param {const char*} content
   * @param {size_t} len
   * @return {bool} 是否成功
   */
  bool SetContent(const char *content, size_t len) {
    std::ofstream ofs;
    ofs.open(filename_.c_str(), std::ios::binary);
    if (!ofs.is_open()) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s open error: %s", filename_.c_str(), strerror(errno));
      return false;
    }
    ofs.write(content, len);
    if (!ofs.good()) {
      mylog::GetLogger("cloud_storage")
          ->Info("%s, file set content error", filename_.c_str());
      ofs.close();
    }
    ofs.close();
    return true;
  }

  //////////////////////////////////////////////
  // 压缩操作
  //  压缩文件
  /**
   * @description: 压缩文件
   * @param {const std::string &} content
   * @return {bool} 是否成功
   */
  bool Compress(const std::string &content) {

    std::string packed = zstd_wrapper::pack(content);
    if (packed.size() == 0) {
      mylog::GetLogger("cloud_storage")
          ->Info("Compress packed size error:%d", packed.size());
      return false;
    }
    // 将压缩的数据写入压缩包文件中
    FileUtil f(filename_);
    if (f.SetContent(packed.c_str(), packed.size()) == false) {
      mylog::GetLogger("cloud_storage")
          ->Info("filename:%s, Compress SetContent error", filename_.c_str());
      return false;
    }
    return true;
  }
  /**
   * @description: 解压缩文件
   * @param {std::string &} download_path
   * @return {bool} 是否成功
   */
  bool UnCompress(std::string &download_path) {
    // 将当前压缩包数据读取出来
    std::string body;
    if (this->GetContent(&body) == false) {
      mylog::GetLogger("cloud_storage")
          ->Info("filename:%s, uncompress get file content failed!",
                 filename_.c_str());
      return false;
    }
    // 对压缩的数据进行解压缩
    std::string unpacked = zstd_wrapper::unpack(body);
    // 将解压缩的数据写入到新文件
    FileUtil fu(download_path);
    if (fu.SetContent(unpacked.c_str(), unpacked.size()) == false) {
      mylog::GetLogger("cloud_storage")
          ->Info("filename:%s, uncompress write packed data failed!",
                 filename_.c_str());
      return false;
    }
    return true;
  }


  bool Exists() { return fs::exists(filename_); }


  bool CreateDirectory() {
    if (Exists())
      return true;
    return fs::create_directories(filename_);
  }


};

class JsonUtil {
public:
  /**
   * @description: 序列化
   * @param {const Json::Value &} val
   * @param {std::string*} str
   * @return {bool} 是否成功
   */
  static bool Serialize(const Json::Value &val, std::string *str) {
    // 建造者生成->建造者实例化json写对象->调用写对象中的接口进行序列化写入str
    Json::StreamWriterBuilder swb;
    swb["emitUTF8"] = true;
    std::unique_ptr<Json::StreamWriter> usw(swb.newStreamWriter());
    std::stringstream ss;
    if (usw->write(val, &ss) != 0) {
      mylog::GetLogger("cloud_storage")->Info("serialize error");
      return false;
    }
    *str = ss.str();
    return true;
  }
  /**
   * @description: 反序列化
   * @param {const std::string &} str
   * @param {Json::Value*} val
   * @return {bool} 是否成功
   */
  static bool UnSerialize(const std::string &str, Json::Value *val) {
    // 操作方法类似序列化
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> ucr(crb.newCharReader());
    std::string err;
    if (ucr->parse(str.c_str(), str.c_str() + str.size(), val, &err) == false) {
      mylog::GetLogger("cloud_storage")->Info("parse error");
      return false;
    }
    return false;
  }
};
} // namespace storage