// 远程备份debug等级以上的日志信息-接收端
#include "ServerBackupLog.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
using std::cout;
using std::endl;
// 定义日志文件的路径和名称
const std::string filename = "./logfile.log";
/**
 * @brief 显示程序使用方法的错误提示
 * @param procgress 程序名称
 *
 * 当用户未正确提供命令行参数时，显示如何正确使用程序的错误信息，
 * 提示用户需要提供端口号参数。
 */
void usage(std::string procgress) {
  cout << "usage error:" << procgress << "port" << endl;
}
/**
 * @brief 检查文件是否存在
 * @param name 文件路径名
 * @return 文件存在返回true，否则返回false
 *
 * 使用stat系统调用检查指定路径的文件是否存在
 */
bool file_exist(const std::string &name) {
  struct stat exist;
  return (stat(name.c_str(), &exist) == 0);
}

/**
 * @brief 将日志消息写入备份文件
 * @param message 需要备份的日志消息
 *
 * 该函数作为TCP服务器的回调函数，当接收到客户端发送的日志消息时被调用。
 * 将接收到的日志消息以二进制追加方式写入到指定的日志文件中。
 * 如果文件操作失败，程序会终止并输出错误信息。
 */
void backup_log(const std::string &message) {
  // 以二进制追加模式打开日志文件
  FILE *fp = fopen(filename.c_str(), "ab");
  if (fp == NULL) {
    perror("fopen error: ");
    assert(false); // 文件打开失败，程序终止
  }

  // 写入日志消息
  int write_byte = fwrite(message.c_str(), 1, message.size(), fp);
  if (write_byte != message.size()) {
    perror("fwrite error: ");
    assert(false); // 写入失败，程序终止
  }

  // 刷新缓冲区并关闭文件
  fflush(fp);
  fclose(fp);
}
/**
 * @brief 主函数 - 日志备份服务器入口
 * @param args 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出状态码
 *
 * 程序流程：
 * 1. 检查命令行参数，必须提供端口号
 * 2. 创建TCP服务器实例，并设置日志备份回调函数
 * 3. 初始化服务器（创建socket、绑定地址、开始监听）
 * 4. 启动服务（接受客户端连接并处理日志备份请求）
 */
int main(int args, char *argv[]) {
  // 检查命令行参数，必须提供端口号
  if (args != 2) {
    usage(argv[0]);
    perror("usage error");
    exit(-1);
  }

  // 从命令行参数获取端口号
  uint16_t port = atoi(argv[1]);

  // 创建TCP服务器实例，并设置backup_log作为回调函数
  std::unique_ptr<TcpServer> tcp(new TcpServer(port, backup_log));

  // 初始化服务器（创建socket、绑定地址、开始监听）
  tcp->init_service();

  // 启动服务（接受客户端连接并处理日志备份请求）
  // 注意：此函数包含无限循环，程序将一直运行直到被强制终止
  tcp->start_service();

  return 0;
}