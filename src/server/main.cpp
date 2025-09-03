#include <iostream>
using std::cout;
using std::endl;
#include "../../log_system/log_src/MyLog.hpp"
#include "../../log_system/log_src/ThreadPool.hpp"

ThreadPool* tp=nullptr; // 在源文件中，"实际的变量定义在这里"
void test() {
    int cur_size = 0;
    int cnt = 1;
    while (cur_size++ < 8) {
        LOGDEBUGDEFAULT("测试日志-%d", cnt++);
        mylog::GetLogger("cloud_storage")->Debug("测试日志-%d", cnt++);
        mylog::GetLogger("cloud_storage")->Error("程序错误-%d", cnt++);
    }
}
void init_thread_pool() {
    tp = new ThreadPool(mylog::util::LogConfig::GetJsonData()->thread_count);
}
int main() {
    cout << "Hello World!" << endl;
    init_thread_pool();
    auto Glb = std::make_shared<mylog::LoggerBuilder>(mylog::LoggerBuilder());
    Glb->BuildName("cloud_storage");
    Glb->BuildLoggerFlush<mylog::StdoutFlush>();
    Glb->BuildLoggerFlush<mylog::FileFlush>("../log_system/logfile/FileFlush.log");
    Glb->BuildLoggerFlush<mylog::RollFileFlush>("../log_system/logfile/RollFile_log",
                                              1024 * 1024);
    mylog::LoggerManager::GetInstance().AddLogger(Glb->Build());
    test();
    delete(tp);
    return 0;
}