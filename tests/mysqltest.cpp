#include <iostream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

/*
启动mysql：service mysql status
*/

using namespace std;
using namespace sql;

int main() {
    try {
        // 创建MySQL连接
        Driver* driver = get_driver_instance();
        Connection* con = driver->connect("tcp://127.0.0.1:3306", "root", "1999223zxc");

        // 连接到test数据库
        con->setSchema("test");

        // 执行一条查询语句
        Statement* stmt = con->createStatement();
        ResultSet* res = stmt->executeQuery("SELECT * FROM user");
        while (res->next()) {
            cout << res->getString("name") << endl;
        }

        // 清理连接资源
        delete res;
        delete stmt;
        delete con;
    } catch (SQLException& e) {
        cout << "SQLException: " << e.getErrorCode() << " " << e.what() << endl;
    }
    return 0;
}