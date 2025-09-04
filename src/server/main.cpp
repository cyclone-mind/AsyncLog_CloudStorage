
#define DEBUG_LOG
#include "Service.hpp"
#include <thread>
using namespace std;

storage::DataManager *data_;
ThreadPool *tp = nullptr;

void service_module() {
  storage::Service s;
  mylog::GetLogger("cloud_storage")->Info("service step in RunModule");
  s.RunModule();
}

void log_system_module_init() {
  tp = new ThreadPool(mylog::util::LogConfig::GetJsonData()->thread_count);
  std::shared_ptr<mylog::LoggerBuilder> Glb(new mylog::LoggerBuilder());
  Glb->BuildName("cloud_storage");
  Glb->BuildLoggerFlush<mylog::RollFileFlush>("../log_system/logfile/RollFile_log",
                                              1024 * 1024);

  mylog::LoggerManager::GetInstance().AddLogger(Glb->Build());
}
int main() {
  log_system_module_init();
  data_ = new storage::DataManager();

  thread t1(service_module);

  t1.join();
  delete (tp);
  return 0;
}