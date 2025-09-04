#pragma once
#include "Util.hpp"
#include <memory>
#include <mutex>
// 该类用于读取配置文件信息
namespace storage
{
    // 懒汉模式
    const char *Config_File = "Storage.conf";
    class Config
    {
    private:
        int server_port_;
        std::string server_ip;
        std::string download_prefix_; // URL路径前缀
        std::string deep_storage_dir_;     // 深度存储文件的存储路径
        std::string low_storage_dir_;     // 浅度存储文件的存储路径
        std::string storage_info_;     // 已存储文件的信息
    private:
        Config()
        {
            if (ReadConfig() == false)
            {
                mylog::GetLogger("cloud_storage")->Fatal("ReadConfig failed");
                return;
            }
            mylog::GetLogger("cloud_storage")->Info("ReadConfig complicate");
        }

    public:
        // 读取配置文件信息
        bool ReadConfig()
        {
            mylog::GetLogger("cloud_storage")->Info("ReadConfig start");

            storage::FileUtil fu(Config_File);
            std::string content;
            if (!fu.GetContent(&content))
            {
                return false;
            }

            Json::Value root;
            storage::JsonUtil::UnSerialize(content, &root); // 反序列化，把内容转成jaon value格式

            // 要记得转换的时候用上asint，asstring这种函数，json的数据类型是Value。
            server_port_ = root["server_port"].asInt();
            server_ip = root["server_ip"].asString();
            download_prefix_ = root["download_prefix"].asString();
            storage_info_ = root["storage_info"].asString();
            deep_storage_dir_ = root["deep_storage_dir"].asString();
            low_storage_dir_ = root["low_storage_dir"].asString();
            
            return true;
        }
        int GetServerPort()
        {
            return server_port_;
        }
        std::string GetServerIp()
        {
            return server_ip;
        }
        std::string GetDownloadPrefix()
        {
            return download_prefix_;
        }
        std::string GetDeepStorageDir()
        {
            return deep_storage_dir_;
        }
        std::string GetLowStorageDir()
        {
            return low_storage_dir_;
        }
        std::string GetStorageInfoFile()
        {
            return storage_info_;
        }

    public:
        // 获取单例类对象 - 使用现代C++的线程安全局部静态变量方式
        static Config* GetInstance()
        {
            // C++11 标准保证局部静态变量的初始化是线程安全的
            // 这种方式比传统的双重检查锁定更简洁、安全且高效
            static Config instance;
            return &instance;  // 返回静态实例的地址，保持向后兼容性
        }
        
        // 禁止拷贝构造和赋值操作
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;
    };
    
    /*
     * 传统的双重检查锁定单例模式（已被注释）：
     * 
     * private:
     *     static std::mutex _mutex;
     *     static Config *_instance;
     * 
     * public:
     *     static Config *GetInstance()
     *     {
     *         if (_instance == nullptr)
     *         {
     *             _mutex.lock();
     *             if (_instance == nullptr)
     *             {
     *                 _instance = new Config();
     *             }
     *             _mutex.unlock();
     *         }
     *         return _instance;
     *     }
     * 
     * // 静态成员初始化
     * std::mutex Config::_mutex;
     * Config *Config::_instance = nullptr;
     * 
     * 传统方式的问题：
     * 1. 需要手动管理线程安全（加锁解锁）
     * 2. 需要手动管理内存（new/delete）
     * 3. 代码复杂度高（双重检查锁定模式）
     * 4. 需要在类外定义静态成员变量
     * 
     * 现代方式的优势：
     * 1. C++11 标准保证局部静态变量初始化的线程安全
     * 2. 自动内存管理，无需手动 delete
     * 3. 代码简洁明了
     * 4. 无需额外的静态成员变量定义
     * 5. 零成本初始化（只有在第一次使用时才创建）
     */
}