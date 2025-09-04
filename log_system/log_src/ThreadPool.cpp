#include "ThreadPool.hpp"
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

ThreadPool::ThreadPool(int min, int max)
    : m_minThread(min), m_maxThread(max), m_idleThread(min), m_curThread(min),
      m_stop(false) {
    cout << "ThreadPool created (min=" << min << ", max=" << max << ")" << endl;
    m_manager = new thread(&ThreadPool::manager, this);
    for (int i = 0; i < min; ++i) {
        thread t(&ThreadPool::worker, this);
        m_workers.insert(make_pair(t.get_id(), std::move(t)));
    }
};

ThreadPool::~ThreadPool() {
    cout << "Destroying ThreadPool..." << endl;
    m_stop.store(true);
    m_condition.notify_all();

    // 等待所有工作线程结束
    for (auto &it : m_workers) {
        thread &t = it.second;
        if (t.joinable()) {
            t.join();
        }
    }

    // 等待管理者线程结束
    if (m_manager && m_manager->joinable()) {
        m_manager->join();
        delete m_manager;
        m_manager = nullptr;
    }

    m_workers.clear();
    cout << "ThreadPool destroyed successfully" << endl;
}
void ThreadPool::addTask(function<void(void)> task) {
    {
        lock_guard<mutex> locker(m_queueMutex);
        m_tasks.emplace(task);
    }
    m_condition.notify_one();
}

void ThreadPool::manager(void) {
    while (!m_stop.load()) {
        this_thread::sleep_for(chrono::seconds(1));
        int idel = m_idleThread.load();
        int cur = m_curThread.load();

        // 线程退出
        if (idel > cur / 2 && cur > m_minThread) {
            // cout << "Manager: Too many idle threads, exiting some threads"
            //      << endl;
            m_exitThread.store(2);
            m_condition.notify_all();
            // 等待直到有线程添加了退出
            while (m_ids.empty() && m_exitThread.load() > 0) {
                this_thread::sleep_for(chrono::milliseconds(10));
            }
            // 清理已退出的线程
            {
                lock_guard<mutex> idLocker(m_idsMutex);
                for (auto id : m_ids) {
                    auto it = m_workers.find(id);
                    if (it != m_workers.end()) {
                        if (it->second.joinable()) {
                            it->second.join();
                        }
                        m_workers.erase(it);
                    }
                }
                m_ids.clear(); // 清空已处理的ID列表
            }
        } else if (idel == 0 && cur < m_maxThread &&
                   !m_stop.load()) { // 添加停止检查
            // cout << "Manager: No idle threads, creating new thread" << endl;
            thread t(&ThreadPool::worker, this);
            m_workers.insert(make_pair(t.get_id(), std::move(t)));
            m_curThread++;
            m_idleThread++;
        }
    }
    cout << "Manager thread exiting" << endl;
}

void ThreadPool::worker(void) {
    thread::id tid = this_thread::get_id();
    cout << "Worker thread " << tid << " started" << endl;

    while (!m_stop.load()) {
        function<void(void)> task;
        {
            unique_lock<mutex> locker(m_queueMutex);
            while (m_tasks.empty() && !m_stop.load()) {
                // cout << "Worker thread " << tid << " waiting for task" << endl;
                m_condition.wait(locker);

                // 检查是否需要退出
                if (m_exitThread.load() > 0) {
                    // cout << "Worker thread " << tid << " received exit signal"
                    //      << endl;
                    m_curThread--;
                    m_idleThread--;
                    m_exitThread--;
                    lock_guard<mutex> idLocker(m_idsMutex);
                    m_ids.emplace_back(tid);
                    // cout << "Worker thread " << tid << " added to exit list"
                    //      << endl;
                    return;
                }
            }

            if (!m_tasks.empty()) {
                // cout << "Worker thread " << tid << " got a task" << endl;
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
        }

        if (task) {
            // cout << "Worker thread " << tid << " executing task" << endl;
            m_idleThread--;
            task();
            m_idleThread++;
            // cout << "Worker thread " << tid << " finished task" << endl;
        }
    }

    cout << "Worker thread " << tid << " exiting" << endl;
}

