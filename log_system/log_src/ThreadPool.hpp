

#ifndef ASYNCLOG_CLOUDSTORAGE_THREADPOOL_HPP
#define ASYNCLOG_CLOUDSTORAGE_THREADPOOL_HPP

#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <map>
#include <future>
#include <memory>
#include <condition_variable>
using namespace std;

class ThreadPool {
public:
    explicit ThreadPool(int min = 2,int max = std::thread::hardware_concurrency());
    ~ThreadPool();

    // 添加任务到任务队列
    void addTask(function<void(void)> task);
    template<typename F, typename... Args>
    auto addTask(F&& f, Args&&... args)->future<typename result_of<F(Args...)>::type> {
        // 1. 创建一个 packaged_task 实例
        using returnType = typename result_of<F(Args...)>::type;
        auto mytask = make_shared<packaged_task<returnType()>> (
            bind(forward<F>(f), forward<Args>(args)...));
        // 2. 得到 future
        future<returnType> res = mytask->get_future();
        // 3. 将 packaged_task 添加到任务队列
        {
            lock_guard<mutex> locker(m_queueMutex);
            m_tasks.emplace([mytask]() mutable { (*mytask)(); });
        }
        m_condition.notify_one();
        // 4. 返回future
        return res;
    }
private:
    void manager(void);
    void worker(void);

private:
    thread* m_manager;                    // 线程池管理者线程
    map<thread::id, thread> m_workers;    // 工作线程容器
    vector<thread::id> m_ids;             // 需要退出的线程ID列表
    atomic_int m_minThread;               // 最小线程数
    atomic_int m_maxThread;               // 最大线程数
    atomic_int m_idleThread;              // 空闲线程数
    atomic_int m_curThread;               // 当前活跃线程数
    atomic_int m_exitThread;              // 需要销毁的线程数
    atomic_bool m_stop;                   // 线程池停止标志
    queue<function<void(void)>> m_tasks;  // 任务队列
    mutex m_queueMutex;                   // 任务队列互斥锁
    mutex m_idsMutex;                     // 线程ID列表互斥锁
    condition_variable m_condition;       // 条件变量


};

#endif // ASYNCLOG_CLOUDSTORAGE_THREADPOOL_HPP
