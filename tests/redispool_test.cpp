#include "../SimConnPool.h"
#include <iostream>
#include <memory>
#include <unistd.h>
#include <thread>


static SimpleConnPool::redisPool* pool = SimpleConnPool::redisConnPoolMgr::GetInstance();

void init() {
    pool->connection("127.0.0.1", 6000);
}

void test_set() {
    std::cout << "test_set begin!" << std::endl;
    SimpleConnPool::redisPool::connPtr conn = pool->get();
    while(conn == nullptr) {
        conn = pool->get();
    }
    conn->setSimple("kkk", "100");
    conn->connBack();       // 需要显示放回链接
    std::cout << "test_set end!" << std::endl;
}

// 测试获取连接向队列插入元素
void test_list() {
    std::cout << "test_list begin!" << std::endl;
    SimpleConnPool::redisPool::connPtr conn = pool->get();
    while(conn == nullptr) {
        conn = pool->get();
    }
    conn->push("list", "1");
    conn->push("list", {"a", "b", "c", "d"});
    std::string res;
    conn->pop("list", res);
    std::cout << "取出的元素：" << res << std::endl;
    conn->connBack();       // 需要显示放回链接
    std::cout << "test_list end!" << std::endl;
}

//测试分布式锁
void test_mutex() {
    std::cout << "test_mutex begin!" << std::endl;
    SimpleConnPool::redisPool::connPtr conn = pool->get();
    while(conn == nullptr) {
        conn = pool->get();
    }
    {    
        std::string lockres = conn->lock();
        std::string res = "";
        conn->getSimple("kkk", res);
        std::cout << "结果： " << res << std::endl;
        //conn->pop("list2", res);
        std::string newval = std::to_string(std::stoi(res) - 1);
        conn->setSimple("kkk", newval);
        conn->unlock(lockres);
    }
    conn->connBack();
    std::cout << "test_mutex end!" << std::endl;
}

// 测试执行lua脚本
void test_exeLua() {
    std::cout << "test_exeLua begin!" << std::endl;
    SimpleConnPool::redisPool::connPtr conn = pool->get();
    while(conn == nullptr) {
        conn = pool->get();
    }
    std::string cmd = "EVAL \"redis.call('SET', KEYS[1], ARGV[1]) local result = redis.call('get',KEYS[1]) return result\" 1 foo1 bar1";
    std::string flag;       // set get为string类型，del为int类型，看redis客户端返回的类型设置
    conn->executeLua(cmd.c_str(), &flag);
    std::cout << "lua执行结果：" << flag << std::endl;
    conn->connBack();
    std::cout << "test_exeLua end!" << std::endl;
}

int run() {
    std::cout << "run begin!" << std::endl;
    {
        SimpleConnPool::redisPool::connPtr conn = pool->get();
        //std::cout << "after get use_count:" << conn.use_count() << std::endl;
        while(conn == nullptr) {
            //std::cout << "too many connection!" <<std::endl;
            //sleep(1);
            conn = pool->get();
        }
        //conn->push("list2", "1");
        // 使用分布式锁
        {    
            std::string lockres = conn->lock();
            std::string res = "";
            conn->getSimple("kkk", res);
            std::cout << "结果： " << res << std::endl;
            //conn->pop("list2", res);
            std::string newval = std::to_string(std::stoi(res) - 1);
            conn->setSimple("kkk", newval);
            conn->unlock(lockres);
        }
        /*
        // 测试执行lua脚本
        std::string flag;
        conn->executeLua("eval \"if redis.call('get',KEYS[1]) == ARGV[1] then return redis.call('del',KEYS[1]) else return 0 end\" 1 redis.lock 980848849", &flag);
        std::cout << conn->getError() << std::endl;
        std::cout << flag << std::endl;
        */
        conn->connBack();
        //std::cout << "end!  " << pool->getFreeNum() << std::endl;
    }
    // 提前结束：必须等待工作连接全部转为空闲时才能停止
    /*
    if(pool->getFreeNum() == 5)
        pool->stop();
    */
   std::cout << "run end!" << std::endl;
    return 0;
}

int main() {
    /*单线程*/
    init();
    // 测试set
    test_set();
    // 测试list
    test_list();
    // 测试分布式锁
    test_mutex();
    // 测试lua脚本
    test_exeLua();

    /*多线程测试*/
    int threadnum = 50;
    std::vector<std::thread> threads(threadnum);
    for(int i = 0; i < threadnum; ++i) {
        threads[i] = std::thread(run);
    }
    for(int i = 0; i < threadnum; ++i) {
        threads[i].join();
    }
    return 0;
}