#ifndef _SYLAR_MYSQLHELPER_H_
#define _SYLAR_MYSQLHELPER_H_


#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "SimConnPool.h"


namespace SimpleConnPool {


class MysqlHelper;
//mysql连接池 单例
typedef ConnPool<MysqlHelper> MysqlPool;
typedef Singleton<MysqlPool> MysqlConnPoolMgr;

/*
mysql相关操作
*/
class MysqlHelper : public std::enable_shared_from_this<MysqlHelper>, public  MysqlPool{
public:
    typedef std::shared_ptr<MysqlHelper> ptr;
    typedef Mutex MutexType;

    MysqlHelper()
            : m_host("")
            , m_pwd("")
            , m_user("") {
        m_con = NULL;
        m_pool = MysqlConnPoolMgr::GetInstance();
        std::cout << "初始化完成！" << std::endl;
    }
    ~MysqlHelper() {
        std::cout << "destory MysqlHelper::~MysqlHelper()" << std::endl;
        simRelease(); 
    }

    // 放回连接
    void connBack() {
        //std::cout << "1111" << std::endl;
        std::shared_ptr<MysqlHelper> x = shared_from_this();
        //std::cout << "connBack use_count:" << x.use_count() << std::endl;
        m_pool->back(x);
    }

    // 连接mysql数据库
    bool simpleConn(const std::string& host, const std::string& user, const std::string& pwd) {
        m_host = host;
        m_user = user;
        m_pwd = pwd;
        sql::Driver* driver = get_driver_instance();
        m_con = driver->connect(m_host.c_str(), m_user.c_str(), m_pwd.c_str());
        if(!m_con) {
            m_error_msg = "m_con is null";
            std::cout << "connection error! errstr:" << m_error_msg << std::endl;
            return false;
        }
        m_error_msg = "ok";
        std::cout << "redis connection is ok!" << std::endl;
        return true;
    }

    void setDatabase(const std::string& database) {
        m_con->setSchema(database.c_str());
    }

    // 通过模板T推导出 T对象包含的成员变量名字，
    // 然后根据这个名字拿到mysql结果的值，再转为原本的型别
    // 反射
    



private:
    // 释放单条连接
    void simRelease() {
        delete m_con;
        delete m_pool;
        m_pool = NULL;
    }

private:
    std::string m_host;
    std::string m_pwd;
    std::string m_user;

    sql::Connection* m_con;

    std::string m_error_msg;

    MysqlPool*  m_pool;
    MutexType   m_mutex; 


};


}
#endif