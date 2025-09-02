#include <iostream>
using std::cout;
using std::endl;
#include "../../log_system/log_src/MyLog.hpp"

void test() {
    int cur_size = 0;
    int cnt = 1;
    while (cur_size++ < 8) {
        LOGDEBUGDEFAULT("测试日志-%d", cnt++);
        mylog::GetLogger("cloud_storage")->Debug("测试日志-%d", cnt++);
    }
}

int main() {
    cout << "Hello World!" << endl;
    auto Glb = std::make_shared<mylog::LoggerBuilder>(mylog::LoggerBuilder());
    Glb->BuildName("cloud_storage");
    Glb->BuildLoggerFlush<mylog::StdoutFlush>();
    Glb->BuildLoggerFlush<mylog::FileFlush>("../log_system/logfile/FileFlush.log");
    mylog::LoggerManager::GetInstance().AddLogger(Glb->Build());
    test();
    return 0;
}