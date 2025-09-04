#pragma once
#include "Config.hpp"
#include <unordered_map>
#include <pthread.h>
namespace storage
{
    // 用作初始化存储文件的属性信息
    typedef struct StorageInfo{                   
        time_t mtime_; // 最近修改时间
        time_t atime_; // 最近访问时间
        size_t fsize_; // 文件大小（字节）
        std::string storage_path_; // 文件存储路径
        std::string url_;          // 请求URL中的资源路径

        bool NewStorageInfo(const std::string &storage_path)
        {
            // 初始化备份文件的信息
            mylog::GetLogger("cloud_storage")->Info("NewStorageInfo start");
            FileUtil f(storage_path);
            if (!f.Exists())
            {
                mylog::GetLogger("cloud_storage")->Info("file not exists");
                return false;
            }
            mtime_ = f.LastAccessTime();
            atime_ = f.LastModifyTime();
            fsize_ = f.FileSize();
            storage_path_ = storage_path;
            // URL实际就是用户下载文件请求的路径
            // 下载路径前缀+文件名
            storage::Config *config = storage::Config::GetInstance();
            url_ = config->GetDownloadPrefix() + f.FileName();
            mylog::GetLogger("cloud_storage")->Info("download_url:%s,mtime_:%s,atime_:%s,fsize_:%d", url_.c_str(),ctime(&mtime_),ctime(&atime_),fsize_);
            mylog::GetLogger("cloud_storage")->Info("NewStorageInfo end");
            return true;
        }
    } StorageInfo; // namespace StorageInfo

    /**
     * 存储信息管理器 - 负责管理文件存储元数据
     * 
     * 功能特点：
     * 1. 内存缓存 + 文件持久化的双层存储架构
     * 2. 读写锁保证线程安全的并发访问
     * 3. JSON序列化实现数据持久化
     * 4. 支持按URL和存储路径查询文件信息
     */
    class DataManager
    {
    private:
        std::string storage_file_;                                    // 存储信息持久化文件路径
        pthread_rwlock_t rwlock_;                                    // 读写锁，支持多读单写的并发控制
        std::unordered_map<std::string, StorageInfo> table_;         // 内存哈希表，以URL为key存储文件信息
        bool need_persist_;                                          // 持久化标志，控制是否需要写入磁盘

    public:
        /**
         * 构造函数 - 初始化存储管理器
         * 
         * 初始化流程：
         * 1. 获取持久化文件路径
         * 2. 初始化读写锁
         * 3. 从文件加载历史数据到内存
         * 4. 启用持久化机制
         */
        DataManager()
        {
            mylog::GetLogger("cloud_storage")->Info("DataManager construct start");
            storage_file_ = storage::Config::GetInstance()->GetStorageInfoFile();  // 从配置获取存储文件路径
            pthread_rwlock_init(&rwlock_, NULL);                      // 初始化读写锁
            need_persist_ = false;                                                 // 初始化期间禁用将文件信息持久化到JSON文件中，避免重复IO
            InitLoad();                                                            // 从文件加载已有数据到内存
            need_persist_ = true;                                                  // 初始化完成后启用持久化
            mylog::GetLogger("cloud_storage")->Info("DataManager construct end");
        }
        /**
         * 析构函数 - 清理资源
         * 销毁读写锁，释放系统资源
         */
        ~DataManager()
        {
            pthread_rwlock_destroy(&rwlock_);  // 销毁读写锁
        }

        /**
         * 初始化加载 - 从持久化文件恢复数据到内存
         * 
         * 数据恢复流程：
         * 1. 检查持久化文件是否存在
         * 2. 读取文件内容并JSON反序列化
         * 3. 将数据逐条插入内存哈希表
         * 
         * @return bool 加载成功返回true，失败返回false
         */
        bool InitLoad() 
        {
            mylog::GetLogger("cloud_storage")->Info("init datamanager");
            FileUtil f(storage_file_);
            
            // 如果持久化文件不存在，说明是首次运行，直接返回成功
            if (!f.Exists()){
                mylog::GetLogger("cloud_storage")->Info("there is no storage file info need to load");
                return true;
            }

            // 读取持久化文件内容
            std::string body;
            if (!f.GetContent(&body))
                return false;

            // JSON反序列化，将文件内容转换为结构化数据
            Json::Value root;
            JsonUtil::UnSerialize(body, &root);
            
            // 遍历JSON数组，将每条记录恢复到内存哈希表
            for (int i = 0; i < root.size(); i++)
            {
                StorageInfo info;
                info.fsize_ = root[i]["fsize_"].asInt();                    // 文件大小
                info.atime_ = root[i]["atime_"].asInt();                    // 访问时间
                info.mtime_ = root[i]["mtime_"].asInt();                    // 修改时间
                info.storage_path_ = root[i]["storage_path_"].asString();   // 存储路径
                info.url_ = root[i]["url_"].asString();                     // 访问URL
                Insert(info);  // 插入到内存哈希表
            }
            return true;
        }

        /**
         * 持久化存储 - 将内存数据同步到文件
         * 
         * 持久化流程：
         * 1. 获取内存中的所有数据
         * 2. 将数据序列化为JSON格式
         * 3. 写入持久化文件
         * 
         * @return bool 存储成功返回true，失败返回false
         */
        bool Storage()
        {
            mylog::GetLogger("cloud_storage")->Info("message storage start");
            
            // 获取内存中的所有存储信息
            std::vector<StorageInfo> arr;
            if (!GetAll(&arr))
            {
                mylog::GetLogger("cloud_storage")->Warn("GetAll fail,can't get StorageInfo");
                return false;
            }

            // 构建JSON数组，将所有存储信息序列化
            Json::Value root;
            for (auto e : arr)
            {
                Json::Value item;
                item["mtime_"] = (Json::Int64)e.mtime_;           // 修改时间
                item["atime_"] = (Json::Int64)e.atime_;           // 访问时间  
                item["fsize_"] = (Json::Int64)e.fsize_;           // 文件大小
                item["url_"] = e.url_.c_str();                    // 访问URL
                item["storage_path_"] = e.storage_path_.c_str();  // 存储路径
                root.append(item);  // 添加到JSON数组
            }

            // 将JSON对象序列化为字符串
            std::string body;
            JsonUtil::Serialize(root, &body);
            mylog::GetLogger("cloud_storage")->Info("new message for StorageInfo:%s", body.c_str());

            // 写入持久化文件
            FileUtil f(storage_file_);
            if (f.SetContent(body.c_str(),body.size()) == false)
                mylog::GetLogger("cloud_storage")->Error("SetContent for StorageInfo Error");

            mylog::GetLogger("cloud_storage")->Info("message storage end");
            return true;
        }

        /**
         * 插入存储信息 - 添加新的文件存储记录
         * 
         * @param info 要插入的存储信息对象
         * @return bool 插入成功返回true，失败返回false
         */
        bool Insert(const StorageInfo &info)
        {
            mylog::GetLogger("cloud_storage")->Info("data_message Insert start");
            
            // 获取写锁，确保插入操作的原子性
            pthread_rwlock_wrlock(&rwlock_);
            table_[info.url_] = info;  // 以URL为key插入哈希表
            pthread_rwlock_unlock(&rwlock_);
            
            // 如果启用了持久化，立即同步到文件
            if (need_persist_ == true && Storage() == false)
            {
                mylog::GetLogger("cloud_storage")->Error("data_message Insert:Storage Error");
                return false;
            }
            
            mylog::GetLogger("cloud_storage")->Info("data_message Insert end");
            return true;
        }

        /**
         * 更新存储信息 - 修改已有的文件存储记录
         * 
         * @param info 要更新的存储信息对象
         * @return bool 更新成功返回true，失败返回false
         */
        bool Update(const StorageInfo &info)
        {
            mylog::GetLogger("cloud_storage")->Info("data_message Update start");
            
            // 获取写锁，确保更新操作的原子性
            pthread_rwlock_wrlock(&rwlock_);
            table_[info.url_] = info;  // 更新哈希表中的记录
            pthread_rwlock_unlock(&rwlock_);
            
            // 立即持久化更新后的数据
            if (Storage() == false)
            {
                mylog::GetLogger("cloud_storage")->Error("data_message Update:Storage Error");
                return false;
            }
            
            mylog::GetLogger("cloud_storage")->Info("data_message Update end");
            return true;
        }
        /**
         * 按URL查询存储信息 - 通过文件访问URL获取存储详情
         * 
         * @param key 文件的访问URL
         * @param info 输出参数，存储查询到的信息
         * @return bool 查询成功返回true，未找到返回false
         */
        bool GetOneByURL(const std::string &key, StorageInfo *info)
        {
            pthread_rwlock_rdlock(&rwlock_);  // 获取读锁，允许并发读取
            
            // 在哈希表中查找URL对应的记录
            if (table_.find(key) == table_.end())
            {
                pthread_rwlock_unlock(&rwlock_);
                return false;  // 未找到记录
            }
            
            *info = table_[key];  // 复制找到的存储信息
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }
        /**
         * 按存储路径查询存储信息 - 通过物理存储路径获取存储详情
         * 
         * 注意：由于存储路径不是哈希表的key，需要遍历查找，性能相对较低
         * 
         * @param storage_path 文件的物理存储路径
         * @param info 输出参数，存储查询到的信息
         * @return bool 查询成功返回true，未找到返回false
         */
        bool GetOneByStoragePath(const std::string &storage_path, StorageInfo *info)
        {
            pthread_rwlock_rdlock(&rwlock_);  // 获取读锁
            
            // 遍历哈希表查找匹配的存储路径
            for (auto e : table_)
            {
                if (e.second.storage_path_ == storage_path)
                {
                    *info = e.second;  // 找到匹配记录
                    pthread_rwlock_unlock(&rwlock_);
                    return true;
                }
            }
            
            pthread_rwlock_unlock(&rwlock_);
            return false;  // 未找到匹配记录
        }
        /**
         * 获取所有存储信息 - 返回内存中的全部文件存储记录
         * 
         * @param arry 输出参数，存储所有存储信息的向量
         * @return bool 始终返回true
         */
        bool GetAll(std::vector<StorageInfo> *arry)
        {
            pthread_rwlock_rdlock(&rwlock_);  // 获取读锁
            
            // 遍历哈希表，将所有存储信息添加到向量中
            for (auto e : table_)
                arry->emplace_back(e.second);
                
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }
    }; // namespace DataManager
}
