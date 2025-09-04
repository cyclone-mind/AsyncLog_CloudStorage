# AsyncLog_CloudStorage

高性能异步日志系统，支持云存储备份功能。

## 项目概述

AsyncLog_CloudStorage 是一个基于 C++17 的高性能日志系统，专为需要高吞吐量和可靠日志持久化的应用程序设计。系统采用异步日志、线程池管理和自动云备份功能。

## 主要特性

- **异步日志记录**：使用专用工作线程进行非阻塞日志操作
- **线程池管理**：可配置线程池大小，高效利用线程资源
- **云存储备份**：自动将日志备份到云存储服务
- **压缩支持**：使用 Zstandard 压缩算法优化日志存储
- **JSON 配置**：灵活的 JSON 格式配置文件
- **Libevent 集成**：高性能事件驱动架构

## 依赖要求

- C++17 兼容编译器
- Libevent 2.1.12+
- JsonCpp 1.9.6+
- Zstandard 1.5.7+

## 构建说明

### 使用 CMake

```bash
mkdir build && cd build
cmake ..
make
```

### 使用 vcpkg

项目使用 vcpkg 进行依赖管理，依赖项在 `vcpkg.json` 中配置。

## 项目结构

```
├── src/           # 主应用程序源代码
│   └── server/   # 服务器实现
├── log_system/   # 核心日志系统
│   └── log_src/  # 日志系统源文件
├── docs/         # 文档
└── test_logger   # 测试可执行文件
```

## 核心组件

- **AsyncLogger**：主日志接口，支持异步操作
- **AsyncWorker**：日志处理工作线程
- **ThreadPool**：线程管理和任务分发
- **AsyncBuffer**：日志消息缓冲区管理
- **LogFlush**：日志持久化和刷新机制
- **SendBackupLog**：云存储备份功能

## 日志系统使用方法

包含必要的头文件并初始化日志系统：

```cpp
#include "MyLog.hpp"

// 配置异步日志器
auto builder = std::make_shared<mylog::LoggerBuilder>();
builder->BuildName("cloud_storage");
builder->BuildLoggerFlush<mylog::FileFlush>("../log_system/logfile/app.log");

mylog::LoggerManager::GetInstance().AddLogger(builder->Build());

// 记录日志消息
mylog::GetLogger("cloud_storage")->Info("应用程序启动");
mylog::GetLogger("cloud_storage")->Warn("警告信息");
mylog::GetLogger("cloud_storage")->Error("发生错误");
```

## 配置说明

服务端配置文件：`./src/server/Storage.conf`

```json
{
    "server_port" : <服务器的端口>,
    "server_ip" : <服务器的ip>, 
    "download_prefix" : <下载的url前缀>, 
    "deep_storage_dir" : <深度存储的目录>,   
    "low_storage_dir" : <普通存储的目录>, 
    "storage_info" : <存储信息的文件路径>
}
```

将`./log_system/log_src/backlog_code`中的`ServerBackupLog.cpp` 和 `ServerBackupLog.hpp` 拷贝到远程备份服务器你想要的目录下，并修改配置文件中的`backup_addr`和`backup_port`为远程备份服务器的ip和端口。

日志系统配置文件：`./log_system/log_src/config.conf`

```json
{
    "buffer_size": 10000000,
    "threshold": 10000000000,
    "linear_growth" : 10000000,
    "flush_log" : 1,
    "backup_addr" : <远程备份服务器的ip>,
    "backup_port" : <远程备份服务器的端口>,
    "thread_count" : 3
}
```
## 许可证

本项目遵循 LICENSE 文件中包含的许可证条款。