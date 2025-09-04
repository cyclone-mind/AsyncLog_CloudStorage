cmake_minimum_required(VERSION 4.0)
project(AsyncLog_CloudStorage)

set(CMAKE_CXX_STANDARD 17)


include_directories(log_system/log_src)
include_directories(src/server)

add_executable(AsyncLog_CloudStorage
        src/server/main.cpp
        src/server/base64.cpp
        log_system/log_src/Level.hpp
        log_system/log_src/Message.hpp
        log_system/log_src/Util.hpp
        log_system/log_src/AsyncBuffer.hpp
        log_system/log_src/Manager.hpp
        log_system/log_src/AsyncLogger.hpp
        log_system/log_src/AsyncWorker.hpp
        log_system/log_src/LogFlush.hpp
        log_system/log_src/MyLog.hpp
        log_system/log_src/ThreadPool.hpp
        log_system/log_src/ThreadPool.cpp
        log_system/log_src/backlog_code/SendBackupLog.hpp)

#find_package 做了什么：
    #搜索库文件：在系统中查找 libevent 的安装位置
    #读取配置：读取 libevent 的 libeventConfig.cmake 文件
    #定义目标：创建 libevent::core、libevent::extra 等 CMake 目标
    #设置变量：设置包含路径、库路径等
#搜索路径包括：
    #系统标准路径（如 /usr/lib、/usr/local/lib）
    #CMAKE_PREFIX_PATH 指定的路径
    #环境变量 LIBEVENT_ROOT 等
find_package(Libevent CONFIG REQUIRED)
target_link_libraries(AsyncLog_CloudStorage PRIVATE libevent::core libevent::extra libevent::pthreads)

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(AsyncLog_CloudStorage PRIVATE JsonCpp::JsonCpp)

find_package(zstd CONFIG REQUIRED)
target_link_libraries(AsyncLog_CloudStorage PRIVATE zstd::libzstd)

# 添加文件系统库和必要的链接标志
target_link_libraries(AsyncLog_CloudStorage PRIVATE stdc++fs)