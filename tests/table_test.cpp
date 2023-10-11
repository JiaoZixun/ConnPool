#include "MySQLTable.h"



struct per {
    int a;
    char b;
    double c;
};


int main() {
    // SimpleConnPool::SET_MEMBER(per, a);
    // SimpleConnPool::SET_MEMBER(per, b);
    // SimpleConnPool::SET_MEMBER(per, c);
    auto t = SimpleConnPool::MyTable();
    t.addMember(SimpleConnPool::SET_MEMBER(per, a));
    t.addMember(SimpleConnPool::SET_MEMBER(per, b));
    t.addMember(SimpleConnPool::SET_MEMBER(per, c));

    auto x = t.getMembers();

    for(auto i:x) {
        std::cout<<i.first<<" "<<i.second<<std::endl;
    }

    return 0;

}
