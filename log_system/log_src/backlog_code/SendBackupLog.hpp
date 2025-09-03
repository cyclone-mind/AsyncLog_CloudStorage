
#ifndef ASYNCLOG_CLOUDSTORAGE_CLIBACKUPLOG_HPP
#define ASYNCLOG_CLOUDSTORAGE_CLIBACKUPLOG_HPP
#include <iostream>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../MyLog.hpp"
#include "Util.hpp"


void send_backlog(const std::string& message) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cout << __FILE__ << __LINE__ << "socket error : " << strerror(errno) << std::endl;
    }
    sockaddr_in server{};
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(mylog::util::LogConfig::GetJsonData()->backup_port);
    server.sin_addr.s_addr = inet_addr(mylog::util::LogConfig::GetJsonData()->backup_addr.c_str());
    int cnt = 5;
    while (-1 == connect(fd, reinterpret_cast<struct sockaddr*>(&server), sizeof(server)))
    {
        std::cout << "正在尝试重连,重连次数还有: " << cnt-- << std::endl;
        if (cnt <= 0)
        {
            std::cout << __FILE__ << __LINE__ << "connect error : " << strerror(errno) << std::endl;
            close(fd);
            perror(nullptr);
            return;
        }
    }
    if (-1 == write(fd, message.c_str(), message.size())) // 发送日志信息到服务器
    {
        std::cout << __FILE__ << __LINE__ << "send to server error : " << strerror(errno) << std::endl;
        perror(nullptr);
    }
    close(fd);
}

#endif // ASYNCLOG_CLOUDSTORAGE_CLIBACKUPLOG_HPP

