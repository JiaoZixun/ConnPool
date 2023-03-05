# Redis连接池



> 封装hiredis的一些基本操作，redishelper类提供包含连接，放回，存取键，push，pop，执行redis语句和执行lua脚本的函数，连接池是类模板，传入相应helper类即可实现多种连接池，后续实现mysql连接池。

## 重新编译项目


基于hiredis，只需要引入头文件即可使用，简单方便，复杂语句使用execute函数直接执行，但是需要根据redis客户端返回值，传入该函数第二个参数（set，get返回ok就传string，del返回1或0就传int）

为了可以执行lua脚本，对hiredis中源码进行了改动，原先是以空格来分割命令的，但是lua脚本需要return时肯定包含空格，需要更改分割规则，增加以双引号分割，详见（[hiredis中lua脚本调用_hiredis lua_suhiymof的博客-CSDN博客](https://blog.csdn.net/suhiymof/article/details/54847818)）

```c++
std::string cmd = "EVAL \"redis.call('SET', KEYS[1], ARGV[1]) local result = redis.call('get',KEYS[1]) return result\" 1 foo1 bar1";
std::string flag;       // set get为string类型，del为int类型，看redis客户端返回的类型设置
conn->executeLua(cmd.c_str(), &flag);
std::cout << "lua执行结果：" << flag << std::endl;
```



帮助大家熟悉C++和redis的基本操作，使用lua脚本实现了简单的分布式锁，可以多线程的访问数据库中某个变量

