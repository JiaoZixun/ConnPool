#ifndef _SYLAR_MYSQLTABLE_H_
#define _SYLAR_MYSQLTABLE_H_

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include <typeinfo>
#include <cxxabi.h>



namespace SimpleConnPool {

class MyTable;
// 宏来访问结构体成员
#define SET_MEMBER(structType, memberName) \
    MyTable::setMember<structType>(#memberName, &structType::memberName)



class MyTable {
public:
    MyTable() {}
    ~MyTable() {}

    template <typename StructType, typename MemberType>
    static std::pair<std::string, std::string> setMember(const char* memberName, MemberType StructType::*memberPtr) {
        //std::cout<<memberName<<" "<<abi::__cxa_demangle(typeid(MemberType).name(),0,0,0 )<<std::endl;
        return std::make_pair<std::string, std::string>(memberName, abi::__cxa_demangle(typeid(MemberType).name(),0,0,0 ));
    }

    void addMember(std::pair<std::string, std::string>&& member) {
        m_table.emplace_back(member);
    }

    std::vector<std::pair<std::string, std::string>>& getMembers() {
        return m_table;
    }

private:
    std::vector<std::pair<std::string, std::string>> m_table;
};



}


#endif