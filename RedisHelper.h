#ifndef _SYLAR__REDISHELPER_H_
#define _SYLAR__REDISHELPER_H_


#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <vector>
#include <list>
#include <sstream>
#include <atomic>
#include "SimConnPool.h"







namespace SimpleConnPool {

// redis状态
enum class RedisState {
    M_REDIS_OK = 0,             //执行成功
    M_CONNECT_FAIL = -1,        //连接redis失败
    M_CONTEXT_ERROR = -2,       //RedisContext返回错误
    M_REPLY_ERROR = -3,         //redisReply错误
    M_EXE_COMMAND_ERROR = -4,   //redis命令执行错误
    M_EXECUTE_FAIL = -5,        //执行失败
    M_RETURN_NULL = -6          //返回结果为空
};





class RedisHelper;
//redis连接池 单例
typedef ConnPool<RedisHelper> redisPool;
typedef Singleton<redisPool> redisConnPoolMgr;

/*
redis相关操作：对hiredis进行二次封装，每个连接包含指向连接池的指针
*/
class RedisHelper : public std::enable_shared_from_this<RedisHelper>, public redisPool {
public:
    typedef std::shared_ptr<RedisHelper> ptr;
    typedef Mutex MutexType;

    RedisHelper() 
            : m_host("")
            , m_port(0)
            , m_pwd("")  {
        m_rdsctx = NULL;
        m_rdsrep = NULL;
        m_pool = redisConnPoolMgr::GetInstance();
        std::cout << "初始化完成！" << std::endl;
    }

    ~RedisHelper() {
        std::cout << "destory RedisHelper::~RedisHelper()" << std::endl;
        simRelease(); 
    }

    // 放回连接
    void connBack() {
        //std::cout << "1111" << std::endl;
        std::shared_ptr<RedisHelper> x = shared_from_this();
        //std::cout << "connBack use_count:" << x.use_count() << std::endl;
        m_pool->back(x);
    }

    // 连接redis数据库，默认pwd为空
    bool simpleConn(const std::string& host, int port, const std::string& pwd="") {
        m_host = host;
        m_port = port;
        m_pwd = pwd;
        m_rdsctx = redisConnect(host.c_str(), m_port);
        if (m_rdsctx == NULL || m_rdsctx->err) {
            if (m_rdsctx) {
                m_error_msg = m_rdsctx->errstr;
                m_state = RedisState::M_CONNECT_FAIL;
                std::cout << "connection error! errstr:" << m_error_msg << std::endl;
                return m_state == RedisState::M_REDIS_OK;
            } 
            else {
                m_error_msg = "context is null";
                m_state = RedisState::M_CONTEXT_ERROR;
                std::cout << "context is null! errstr:" << m_error_msg << std::endl;
                return m_state == RedisState::M_REDIS_OK;
            }
        }
        m_error_msg = "ok";
        m_state = RedisState::M_REDIS_OK;
        std::cout << "redis connection is ok!" << std::endl;
        return m_state == RedisState::M_REDIS_OK;
    }

    
    
    // 简单键的存取
    RedisState setSimple(const std::string& key, const std::string& value) {
        return setHandle(key, value);
    }
    RedisState getSimple(const std::string& key, std::string& value) {
        return getHandle(key, value);
    }
    RedisState delSimple(const std::string& key) {
        std::string cmd = "del " +  key;
        int flag = 0;
        execute(cmd, &flag);
        if(!flag && m_state == RedisState::M_REDIS_OK) {
            m_state = RedisState::M_EXECUTE_FAIL;
        }
        return m_state;
    }
    
    // 获取全部key
    std::vector<std::string> getAllKeys() {
        std::string cmd = "keys *";
        int len = 0;
        redisReply **array;
        execute(cmd, &len, &array);
        if(!len && m_state == RedisState::M_REDIS_OK) {
            m_state = RedisState::M_EXECUTE_FAIL;
        }
        std::vector<std::string> res;
        for (int i = 0; i < len; i++) {
            res.emplace_back(array[i]->str);
        }
        return res;
    }
    
    // 超时时间相关操作
    // 设置
    // type控制以秒为单位或以毫秒为单位，使用毫秒时type置为p即可
    bool setKeyTimeout(const std::string& key, uint64_t timeout, const std::string& type = "") {
        //expire 键名 时间  # 秒
        //pexpire 键名 时间 # 毫秒
        // redis返回数字1表示成功
        std::string cmd = type + "expire " +  key + " " + std::to_string(timeout);
        int flag = 0;
        execute(cmd, &flag);
        if(!flag && m_state == RedisState::M_REDIS_OK) {
            m_state = RedisState::M_EXECUTE_FAIL;
        }
        return m_state == RedisState::M_REDIS_OK;
    }

    // 移除
    bool delKeyTimeout(const std::string& key) {
        // persist 键名
        // redis返回数字1表示成功
        std::string cmd = "persist " +  key;
        int flag = 0;
        execute(cmd, &flag);
        if(!flag && m_state == RedisState::M_REDIS_OK) {
            m_state = RedisState::M_EXECUTE_FAIL;
        }
        return m_state == RedisState::M_REDIS_OK;
    }

    // 返回剩余
    uint64_t getKeyTimeout(const std::string& key, const std::string& type = "") {
        //ttl 键名		# 秒
        //pttl 键名		# 毫秒
        // redis返回剩余时间
        std::string cmd = type + "ttl " +  key;
        int timeout;
        execute(cmd, &timeout);
        if(!timeout && m_state == RedisState::M_REDIS_OK) {
            //std::cout << "getKeyTimeout error" << std::endl;
            m_state = RedisState::M_EXECUTE_FAIL;
            //return timeout;
        }
        return timeout;
    }

    // 键是否存在
    bool isExists(const std::string& key) {
        // exists 键名
        // redis返回数字1表示成功
        std::string cmd = "exists " +  key;
        int flag = 0;
        execute(cmd, &flag);
        if(!flag && m_state == RedisState::M_REDIS_OK) {
            m_state = RedisState::M_EXECUTE_FAIL;
        }
        return m_state == RedisState::M_REDIS_OK;
    }

    // 改名
    bool reName(const std::string& old_key, const std::string& new_key) {
        // rename 键名 新键名
        // 返回OK
        std::string cmd = "rename " +  old_key + " " + new_key;
        return execute(cmd) == RedisState::M_REDIS_OK; 
    }

    // 获取错误信息
    const std::string& getError() const {
        return m_error_msg;
    }



// 队列操作
public:
    RedisState pop(const std::string& key, std::string& value) {
        return lpop(key, value);
    }

    RedisState lpop(const std::string& key, std::string& value) {
        std::string cmd = "lpop " + key;
        return execute(cmd, &value);
    }

    RedisState rpop(const std::string& key, std::string& value) {
        std::string cmd = "rpop " + key;
        return execute(cmd, &value);
    }

    RedisState push(const std::string& key, const std::string& value) {
        return rpush(key, value);
    }

    RedisState push(const std::string& key, const std::vector<std::string>& vec) {
        return rpush(key, vec);
    }

    RedisState lpush(const std::string& key, const std::string& value) {
        std::string cmd = "lpush " + key + " " + value;
        int len = 0;
        return execute(cmd, &len);
    }

    RedisState lpush(const std::string& key, const std::vector<std::string>& vec) {
        std::string cmd = "lpush " + key;
        for(auto x:vec) {
            cmd += " " + x;
        }
        int len = 0;
        return execute(cmd, &len);
    }

    RedisState rpush(const std::string& key, const std::string& value) {
        std::string cmd = "rpush " + key + " " + value;
        int len = 0;
        return execute(cmd, &len);
    }

    RedisState rpush(const std::string& key, const std::vector<std::string>& vec) {
        std::string cmd = "rpush " + key;
        for(auto x:vec) {
            cmd += " " + x;
        }
        int len = 0;
        return execute(cmd, &len);
    }

public:
    RedisState parseRdsrep(void* result=NULL, redisReply*** array = NULL) {
        if (m_rdsctx->err) {
            m_error_msg = m_rdsctx->errstr;
            m_state = RedisState::M_CONTEXT_ERROR;
            return m_state;
        }

        if (m_rdsrep == NULL) {
            m_error_msg = "auth redisReply is NULL";
            m_state = RedisState::M_REPLY_ERROR;
            return m_state;
        }
        //std::cout << "type: " << m_rdsrep->type << std::endl;
        switch (m_rdsrep->type){
            case REDIS_REPLY_ERROR:
                m_error_msg = m_rdsrep->str;
                m_state = RedisState::M_EXE_COMMAND_ERROR;
                return m_state;
            case REDIS_REPLY_STATUS:
                if (strcmp(m_rdsrep->str, "OK") == 0) {
                    m_state = RedisState::M_REDIS_OK;
                    //*(std::string*)result = m_rdsrep->str;
                    return m_state;
                }
                else {
                    m_error_msg = m_rdsrep->str;
                    m_state =  RedisState::M_EXE_COMMAND_ERROR;
                    return m_state;
                }
            case REDIS_REPLY_INTEGER:
                //std::cout<<"REDIS_REPLY_INTEGER " << m_rdsrep->type <<std::endl;
                *(int*)result = m_rdsrep->integer;
                m_state = RedisState::M_REDIS_OK;
                return m_state;
            case REDIS_REPLY_STRING:
                *(std::string*)result = m_rdsrep->str;
                m_state = RedisState::M_REDIS_OK;
                return m_state;
            case REDIS_REPLY_NIL:
                *(std::string*)result = "";
                m_state = RedisState::M_RETURN_NULL;
                return m_state;
            case REDIS_REPLY_ARRAY:
                *(int*)result = m_rdsrep->elements;
                *array = m_rdsrep->element;
                m_state = RedisState::M_REDIS_OK;
                return m_state;
            default:
                m_error_msg = "unknow reply type";
                m_state =  RedisState::M_EXE_COMMAND_ERROR;
                return m_state;
        }
    }
    // 执行语句
    RedisState execute(const std::string& cmd, void* result=NULL, redisReply*** array = NULL) {
        //std::cout << "lua:" << cmd << std::endl;
        m_rdsrep = (redisReply*)redisCommand(m_rdsctx, cmd.c_str());
        return parseRdsrep(result, array);
    }

    RedisState executeLua(const std::string& cmd, void* result=NULL, redisReply*** array = NULL) {
        redisAppendCommand(m_rdsctx, cmd.c_str());
        if (redisGetReply(m_rdsctx, (void**)&m_rdsrep) != REDIS_OK)
            return RedisState::M_EXE_COMMAND_ERROR;
        return parseRdsrep(result, array);
    }

public:
    // 分布式锁 TODO
    /*
    在分布式系统环境下，一个方法在同一时间只能被一个机器的一个线程执行
    高可用的获取锁与释放锁
    高性能的获取锁与释放锁
    具备可重入特性（可理解为重新进入，由多于一个任务并发使用，而不必担心数据错误）
    具备锁失效机制，即自动解锁，防止死锁
    具备非阻塞锁特性，即没有获取到锁将直接返回获取锁失败
    */

    /* 
        加锁逻辑：
        在不存在键时设置键 并同时设置超时时间  保证原子性
        set key value NX PX timeout
        默认10秒
        返回该线程拿到锁生成的值，后续解锁比较，不能因为挂机或网络原因解锁别的线程锁
        分布式锁的键为：redis.lock      值为：当前时间戳 nsec
    */
    std::string lock(uint64_t timeout = 30000) {
        while(1) {
            struct timespec time = {0, 0};
            clock_gettime(CLOCK_REALTIME, &time);
            std::string uuid = std::to_string(time.tv_nsec);
            std::string cmd = "set redis.lock " + uuid + " NX PX " + std::to_string(timeout);
            //std::cout << "lock cmd:" << cmd << std::endl;
            std::string flag;
            RedisState sta;
            {
                MutexType::Lock lock(m_mutex);
                sta = execute(cmd, &flag);
            }
            if(sta == RedisState::M_REDIS_OK) {
                std::cout << "get redis.lock!" << std::endl;
                return uuid;
            }
        }
    }

    /*
        解锁逻辑：
        获取锁，判断锁，删除锁
        为了保证原子性，使用lua脚本来完成三个操作
        redis使用eval "xxxx" keys数量 key1 key2 num1 num2 
    */
    void unlock(const std::string& uuid) {
        std::string cmd = "eval \"if redis.call('get',KEYS[1]) == ARGV[1] then return redis.call('del',KEYS[1]) else return 0 end\" 1 redis.lock " + uuid;
        //std::cout << "unlock cmd " << cmd << std::endl;
        int flag;
        RedisState sta = executeLua(cmd.c_str(), &flag);
        if(sta == RedisState::M_REDIS_OK && flag > 0) {
            std::cout << "获取到锁并解锁" << std::endl;
        }
        else {
            std::cout << "未获取到锁 errstr: " << m_error_msg << std::endl;
        }
        return ;
    }


private:
    // 释放单条连接
    void simRelease() {
        freeReplyObject(m_rdsrep);
        redisFree(m_rdsctx);
        m_rdsctx = NULL;
        m_rdsrep = NULL;
        m_pool = NULL;
    }
    // 存值操作
    RedisState setHandle(const std::string& key, const std::string& value) {
        std::string cmd = "set " + key + " " + value;
        return execute(cmd);
    }
    // 取值操作
    RedisState getHandle(const std::string& key, std::string& value) {
        std::string cmd = "get " + key;
        return execute(cmd, &value);
    }

    
    
private:
    std::string m_host;                 //连接地址
    int m_port;                         //端口号
    std::string m_pwd;                  //密码
    /*
    typedef struct redisContext {
        int err; 
        char errstr[128]; 
        int fd;
        int flags;
        char *obuf; 
        redisReader *reader; 
        enum redisConnectionType connection_type;
        struct timeval *timeout;
        struct {
            char *host;
            char *source_addr;
            int port;
        } tcp;
        struct {
            char *path;
        } unix_sock;
    } redisContext;

    */
    redisContext *m_rdsctx;             //redis结构体
    /*
    typedef struct redisReply {
        //命令执行结果的返回类型
        int type; 
        //存储执行结果返回为整数
        long long integer; 
        //字符串值的长度
        size_t len; 
        //存储命令执行结果返回是字符串
        char *str; 
        //返回结果是数组的大小
        size_t elements; 
        //存储执行结果返回是数组
        struct redisReply **element;
    } redisReply

    */
    redisReply *m_rdsrep;               //返回结果结构体
    
    std::string m_error_msg;            //错误信息
    RedisState  m_state;                //状态码

    redisPool*  m_pool;                 // 指向连接池
    MutexType   m_mutex;                // 互斥锁

};

//mysql连接池 单例
typedef ConnPool<MysqlHelper> mysqlPool;
typedef Singleton<mysqlPool> mysqlConnPoolMgr;

/*
mysql相关操作
*/
class MysqlHelper : public std::enable_shared_from_this<MysqlHelper> {
public:
    typedef std::shared_ptr<MysqlHelper> ptr;

//TODO

};



}

#endif