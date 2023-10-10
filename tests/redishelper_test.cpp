#include "../RedisHelper.h"
#include <iostream>
#include <memory>
#include <unistd.h>
#include <thread>


int run(const std::string& threadname = "") {
    std::cout << "进入线程: " << threadname << std::endl;
    // conn
    // sylar::RedisHelper::ptr rediscon = std::make_shared<sylar::RedisHelper>();
    auto rediscon = SimpleConnPool::RedisHelper::ptr(new SimpleConnPool::RedisHelper());
    if(!rediscon->simpleConn("127.0.0.1", 6000)) {
        std::cout << "redis connection error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }

    /*
    // set
    sylar::RedisState replystate = rediscon->setSimple("test", "edf");
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    replystate = rediscon->setSimple("test1", "abc");
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    replystate = rediscon->setSimple("test2", "abc");
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }

    replystate = rediscon->setSimple("test3", "111");
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }



    // get
    std::string result;
    replystate = rediscon->getSimple("test", result);
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    std::cout << "test: " << result << std::endl;

    std::string result1;
    replystate = rediscon->getSimple("test1", result1);
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    std::cout << "test1: " << result1 << std::endl;

    std::string result2;
    replystate = rediscon->getSimple("test2", result2);
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    std::cout << "test2: " << result2 << std::endl;

    std::string result3;
    replystate = rediscon->getSimple("test3", result3);
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    std::cout << "test3: " << result3 << std::endl;


    std::string result4;
    replystate = rediscon->getSimple("test4", result4);
    if(replystate != sylar::RedisState::M_REDIS_OK) {
        std::cout << "redis reple error! errstr:" << rediscon->getError() << std::endl;
        return 0;
    }
    std::cout << "test4: " << result4 << std::endl;

    std::cout << "之前：" << std::endl;
    std::vector<std::string> res = rediscon->getAllKeys();
    for(auto x:res) {
        std::cout << x << " ";
    }
    std::cout << std::endl;


    // 设置超时时间
    if(rediscon->setKeyTimeout("test", 100)) {
        std::cout << "设置超时时间" << std::endl;
        sleep(10);
        // 获取超时时间
        int64_t timeout = rediscon->getKeyTimeout("test");
        std::cout << "剩余超时时间:" << timeout << std::endl;

        if(rediscon->delKeyTimeout("test")) {
            std::cout << "移除超时时间" << std::endl;
        }
    }

    // 是否存在键
    if(rediscon->isExists("test")) {
        std::cout << "存在" << std::endl;
    }
    else {
        std::cout << "不存在" << std::endl;
    }

    // 改名
    if(rediscon->reName("test", "test")) {
        std::cout << "test -> test  成功" << std::endl;
    }
    else {
        std::cout << "test -> test  失败" << std::endl;
    }

    if(rediscon->reName("test", "test5")) {
        std::cout << "test -> test5  成功" << std::endl;
    }
    else {
        std::cout << "test -> test5  失败" << std::endl;
    }

    rediscon->delSimple("test1");

    std::cout << "之前：" << std::endl;
    res = rediscon->getAllKeys();
    for(auto x:res) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
    */

    rediscon->push("list1", "a");
    rediscon->push("list1", "b");
    rediscon->push("list1", "c");
    rediscon->push("list1", "d");

    rediscon->push("list2", {"1", "2", "3", "4", "5"});
    /*
    while(1) {
        std::string res = "";
        rediscon->pop("list1", res);
        if(res != "") {
            std::cout << threadname << " 存在数据：" << res << std::endl;
        }
    }
    */
    /*
    auto func = [](sylar::RedisHelper::ptr rediscon, const std::string& thread_name){
        while(1) {
            std::string res = "";
            rediscon->pop("list1", res);
            if(res != "") {
                std::cout << thread_name << "中存在数据：" << res << std::endl;
            }
        }
    };
    std::thread t1(func, rediscon, "t1");
    std::thread t2(func, rediscon, "t2");
    t1.join();
    t2.join();
    */
    std::cout << "finish!" << std::endl;
    std::cout << "rediscon引用计数： " << rediscon.use_count() << std::endl;
    rediscon->connBack();
    std::cout << "离开线程" << threadname << std::endl;
    return 0;
}

int main() {
    std::cout << "start！" << std::endl;
    /*
    std::thread t1(run, "t1");
    std::thread t2(run, "t2");
    t1.join();
    t2.join();
    */
    run();
    std::cout << "end" << std::endl;
    return 0;
}