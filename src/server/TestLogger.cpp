#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <fstream>
#include <memory>
using std::cout;
using std::endl;
#include "../../log_system/log_src/MyLog.hpp"
#include "../../log_system/log_src/ThreadPool.hpp"
#include <chrono>

ThreadPool *tp = nullptr; // 在源文件中，"实际的变量定义在这里"

class Timer {
  private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;

  public:
    void start() { start_time = std::chrono::high_resolution_clock::now(); }
    void stop() { end_time = std::chrono::high_resolution_clock::now(); }
    double getDurationMs() const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        return duration.count() / 1000.0; // 转换为毫秒
    }

    double getDurationS() const {
        return getDurationMs() / 1000.0; // 转换为秒
    }
};

void init_thread_pool() {
    tp = new ThreadPool(mylog::util::LogConfig::GetJsonData()->thread_count);
}

// 简单的同步日志器类
class SyncLogger {
public:
    SyncLogger(const std::string& filename) : filename_(filename) {
        // 确保目录存在
        auto pos = filename_.find_last_of("/");
        if (pos != std::string::npos) {
            std::string dir = filename_.substr(0, pos);
            system(("mkdir -p " + dir).c_str());
        }
        
        file_.open(filename_, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "Failed to open sync log file: " << filename_ << std::endl;
        }
    }
    
    ~SyncLogger() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void InfA(const char* format, ...) {
        if (!file_.is_open()) return;
        
        va_list args;
        va_start(args, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);
        
        // 格式化日志消息
        char time_buffer[64];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &tm_now);
        
        file_ << "[" << time_buffer << "] [INFO] " << buffer << std::endl;
        file_.flush(); // 同步写入，立即刷盘
    }
    
private:
    std::string filename_;
    std::ofstream file_;
};

// 计算日志消息的平均大小（字节）
size_t calculate_log_message_size(const std::string& prefix, int index) {
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%s log test %d", prefix.c_str(), index);
    return len > 0 ? static_cast<size_t>(len) : 0;
}

// 测试吞吐量比较
void test_throughput_comparison(int log_count) {
    cout << "\n===== 异步 vs 同步日志系统吞吐量比较 =====" << endl;
    cout << "日志数量: " << log_count << " 条" << endl;
    
    // 预先计算平均日志大小
    size_t total_bytes = 0;
    for (int i = 0; i < std::min(1000, log_count); i++) {
        total_bytes += calculate_log_message_size("Async", i);
    }
    double avg_log_size_bytes = static_cast<double>(total_bytes) / std::min(1000, log_count);
    double total_data_mb = (log_count * avg_log_size_bytes) / (1024.0 * 1024.0);
    
    cout << "平均日志大小: " << std::fixed << std::setprecision(2) << avg_log_size_bytes << " 字节" << endl;
    cout << "总数据量: " << std::fixed << std::setprecision(3) << total_data_mb << " MB" << endl;
    
    // 异步日志测试
    cout << "\n--- 异步日志系统 ---" << endl;
    Timer async_timer;
    async_timer.start();
    
    for (int i = 0; i < log_count; i++) {
        mylog::GetLogger("cloud_storage")->Info("Async log test %d", i);
    }
    
    async_timer.stop();
    
    double async_duration_s = async_timer.getDurationS();
    double async_throughput_logs = log_count / async_duration_s;
    double async_throughput_mb = total_data_mb / async_duration_s;
    
    cout << "耗时: " << std::fixed << std::setprecision(3) << async_duration_s << " 秒" << endl;
    cout << "吞吐量: " << std::fixed << std::setprecision(0) << async_throughput_logs << " 条/秒" << endl;
    cout << "吞吐量: " << std::fixed << std::setprecision(2) << async_throughput_mb << " MB/秒" << endl;
    cout << "平均延迟: " << std::fixed << std::setprecision(4) 
         << (async_timer.getDurationMs() / log_count) << " ms/条" << endl;
    
    // 等待异步日志处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 同步日志测试
    cout << "\n--- 同步日志系统 ---" << endl;
    SyncLogger sync_logger("../log_system/logfile/sync_throughput_test.log");
    
    Timer sync_timer;
    sync_timer.start();
    
    for (int i = 0; i < log_count; i++) {
        sync_logger.InfA("Sync log test %d", i);
    }
    
    sync_timer.stop();
    
    double sync_duration_s = sync_timer.getDurationS();
    double sync_throughput_logs = log_count / sync_duration_s;
    double sync_throughput_mb = total_data_mb / sync_duration_s;
    
    cout << "耗时: " << std::fixed << std::setprecision(3) << sync_duration_s << " 秒" << endl;
    cout << "吞吐量: " << std::fixed << std::setprecision(0) << sync_throughput_logs << " 条/秒" << endl;
    cout << "吞吐量: " << std::fixed << std::setprecision(2) << sync_throughput_mb << " MB/秒" << endl;
    cout << "平均延迟: " << std::fixed << std::setprecision(4) 
         << (sync_timer.getDurationMs() / log_count) << " ms/条" << endl;
    
    // 性能比较
    cout << "\n--- 性能比较 ---" << endl;
    double throughput_logs_ratio = async_throughput_logs / sync_throughput_logs;
    double throughput_mb_ratio = async_throughput_mb / sync_throughput_mb;
    double speedup = sync_duration_s / async_duration_s;
    
    cout << "异步/同步吞吐量比 (条/秒): " << std::fixed << std::setprecision(2) << throughput_logs_ratio << "x" << endl;
    cout << "异步/同步吞吐量比 (MB/秒): " << std::fixed << std::setprecision(2) << throughput_mb_ratio << "x" << endl;
    cout << "速度提升: " << std::fixed << std::setprecision(2) << speedup << "x" << endl;
    cout << "异步延迟降低: " << std::fixed << std::setprecision(2) 
         << ((sync_timer.getDurationMs() / log_count) / (async_timer.getDurationMs() / log_count)) << "x" << endl;
}

// 测试多线程并发吞吐量比较
void test_concurrent_throughput_comparison(int thread_count, int logs_per_thread) {
    cout << "\n===== 多线程并发吞吐量比较 =====" << endl;
    cout << "线程数: " << thread_count << endl;
    cout << "每线程日志数: " << logs_per_thread << endl;
    cout << "总日志数: " << thread_count * logs_per_thread << " 条" << endl;
    
    // 异步日志多线程测试
    cout << "\n--- 异步日志系统 (多线程) ---" << endl;
    Timer async_timer;
    async_timer.start();
    
    std::vector<std::thread> async_threads;
    for (int t = 0; t < thread_count; t++) {
        async_threads.emplace_back([t, logs_per_thread]() {
            for (int i = 0; i < logs_per_thread; i++) {
                mylog::GetLogger("cloud_storage")->Info("Async Thread-%d Log-%d", t, i);
            }
        });
    }
    
    for (auto& t : async_threads) {
        t.join();
    }
    
    async_timer.stop();
    
    int total_logs = thread_count * logs_per_thread;
    double async_duration_s = async_timer.getDurationS();
    double async_throughput = total_logs / async_duration_s;
    
    cout << "耗时: " << std::fixed << std::setprecision(3) << async_duration_s << " 秒" << endl;
    cout << "总吞吐量: " << std::fixed << std::setprecision(0) << async_throughput << " 条/秒" << endl;
    
    // 等待异步日志处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 同步日志多线程测试
    cout << "\n--- 同步日志系统 (多线程) ---" << endl;
    Timer sync_timer;
    sync_timer.start();
    
    std::vector<std::thread> sync_threads;
    for (int t = 0; t < thread_count; t++) {
        sync_threads.emplace_back([t, logs_per_thread]() {
            SyncLogger sync_logger("../log_system/logfile/sync_concurrent_test.log");
            for (int i = 0; i < logs_per_thread; i++) {
                sync_logger.InfA("Sync Thread-%d Log-%d", t, i);
            }
        });
    }
    
    for (auto& t : sync_threads) {
        t.join();
    }
    
    sync_timer.stop();
    
    double sync_duration_s = sync_timer.getDurationS();
    double sync_throughput = total_logs / sync_duration_s;
    
    cout << "耗时: " << std::fixed << std::setprecision(3) << sync_duration_s << " 秒" << endl;
    cout << "总吞吐量: " << std::fixed << std::setprecision(0) << sync_throughput << " 条/秒" << endl;
    
    // 性能比较
    cout << "\n--- 多线程性能比较 ---" << endl;
    double throughput_ratio = async_throughput / sync_throughput;
    double speedup = sync_duration_s / async_duration_s;
    
    cout << "异步/同步吞吐量比: " << std::fixed << std::setprecision(2) << throughput_ratio << "x" << endl;
    cout << "速度提升: " << std::fixed << std::setprecision(2) << speedup << "x" << endl;
}

int main() {
    cout << "========== 异步 vs 同步日志系统吞吐量测试 ==========" << endl;
    cout << "初始化线程池..." << endl;
    init_thread_pool();
    
    // 配置异步日志器
    auto builder = std::make_shared<mylog::LoggerBuilder>();
    builder->BuildName("cloud_storage");
    builder->BuildLoggerFlush<mylog::FileFlush>("../log_system/logfile/async_throughput_test.log");
    
    mylog::LoggerManager::GetInstance().AddLogger(builder->Build());
    
    cout << "日志器配置完成，开始测试..." << endl;
    
    // 1. 单线程吞吐量比较测试
    test_throughput_comparison(10000);
    
    // 2. 多线程并发吞吐量比较测试
    test_concurrent_throughput_comparison(4, 2500);  // 4线程，每线程2500条
    
    // 等待所有异步日志写入完成
    cout << "\n等待异步日志写入完成..." << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    cout << "\n测试完成！" << endl;
    cout << "异步日志文件: ../log_system/logfile/async_throughput_test.log" << endl;
    cout << "同步日志文件: ../log_system/logfile/sync_throughput_test.log" << endl;
    cout << "多线程同步日志文件: ../log_system/logfile/sync_concurrent_test.log" << endl;
    
    delete (tp);
    return 0;
}