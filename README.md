# Redis连接池



> 封装hiredis的一些基本操作，redishelper类提供包含连接，放回，存取键，push，pop，执行redis语句和执行lua脚本的函数，连接池是类模板，传入相应helper类即可实现多种连接池，后续实现mysql连接池。

## 编译项目

1. 安装hiredis，解压文件中hiredis.tar（后续对源码进行了改动，文件中是改后的）

```shell
tar -xvf hiredis.tar
cd hiredis/
make
make install	# 权限不足加上sudo
```

2. 删除之前的CMakeCache.txt

```shell
cd ConnPool/
rm CMakeCache.txt
```

3. 重新cmake和make

```shell
cmake .
make -j4
```

4. 后续引入头文件SimConnPool.h即可使用，简单应用见tests文件

## 整体架构
1. RedisHelper：redis相关操作，创建一个连接时构造函数不做任何事，在connection函数时才真正建立一条连接，每条连接都包含一个指向连接池的指针，为了实现放回操作，基础实现依赖hiredis库中的redisCommand函数
2. ConnPool ：连接池模板，初始化时构造函数不做任何操作，在连接池connection时才执行init函数完成每一个连接的初始化，并且依次调用每条连接的connection函数
3. Singleton：单例模板，包装ConnPool类，变成单例对象


## 使用

```c++
static SimpleConnPool::redisPool* pool = SimpleConnPool::redisConnPoolMgr::GetInstance();   // 获取一个连接池单例对象，默认5个连接
pool->connection("127.0.0.1", 6000);      // 连接池内连接上redis，入参：ip、端口号、密码默认为空
SimpleConnPool::redisPool::connPtr conn = pool->get();      // 取出一条连接
// TODO
conn->connBack();     // 放回连接

```

## 分布式锁

> 当多线程并发访问数据库中某个资源时，可能同一时间返回都取到了旧值，并且对旧值进行变更，这样会造成逻辑上的混淆。比如在电商场景，库存这个变量，当多个用户进行抢购的时候，首先判断库存然后库存减一，若不加锁，库存为1的时候，可能多个用户获取库存判断时都拿到了1，最后都进行库存减一，可能就出现负数了。

  在分布式系统环境下，一个方法在同一时间只能被一个机器的一个线程执行

  高可用的获取锁与释放锁

  具备可重入特性（可理解为重新进入，由多于一个任务并发使用，而不必担心数据错误）

  具备锁失效机制，即自动解锁，防止死锁



加锁：

实际是，对一个键进行赋值，借助set key val NX PX timeout 语句实现，NX表示若键存在返回nil，不存在则赋值，当多线程运行到lock时，只有第一个执行语句的线程可以跳出，继续执行，其余线程都需要阻塞在lock内部。

设置超时时间为了保证不会因为加锁线程挂掉而导致死锁的情况。

```c++
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
```

解锁：

需要取值、判断、删除三个操作，因此使用lua脚本来保证原子性，判断是为了保证不会删除其余线程所创建的锁。

```c++
/*
        解锁逻辑：
        获取锁，判断锁，删除锁
        为了保证原子性，使用lua脚本来完成三个操作
        redis使用eval "xxxx" keys数量 key1 key2 num1 num2 
    */
void unlock(const std::string& uuid) {
    std::string cmd = "eval \"if redis.call('get',KEYS[1]) == ARGV[1] then return redis.call('del',KEYS[1]) else return 0 end\" 1 redis.lock " + uuid;
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
```



## 总结

基于hiredis，只需要引入头文件即可使用，简单方便，复杂语句使用execute函数直接执行，但是需要根据redis客户端返回值，传入该函数第二个参数（set，get返回ok就传string，del返回1或0就传int）

为了可以执行lua脚本，对hiredis中源码进行了改动，原先是以空格来分割命令的，但是lua脚本需要return时肯定包含空格，需要更改分割规则，增加以双引号分割，详见（[hiredis中lua脚本调用_hiredis lua_suhiymof的博客-CSDN博客](https://blog.csdn.net/suhiymof/article/details/54847818)）

```c++
std::string cmd = "EVAL \"redis.call('SET', KEYS[1], ARGV[1]) local result = redis.call('get',KEYS[1]) return result\" 1 foo1 bar1";
std::string flag;       // set get为string类型，del为int类型，看redis客户端返回的类型设置
conn->executeLua(cmd.c_str(), &flag);
std::cout << "lua执行结果：" << flag << std::endl;
```



帮助大家熟悉C++和redis的基本操作，使用lua脚本实现了简单的分布式锁，可以多线程的访问数据库中某个变量

还缺少健康检测，自动扩缩容等实现，大家有需要可以继续开发













