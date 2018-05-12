//
// Created by 钱诚亮 on 2018/5/12.
//



#ifndef MINISQL_Attribute_h
#define MINISQL_Attribute_h

#include <string>
#include <iostream>
using namespace std;

class Attribute
{
public:
    string name;
    int type;
    //属性类型
    // -1 代表 浮点型
    //  0 代表 整型
    // 其他正数 i 代表 char(i)
    bool ifUnique;
    string index;         // default value is "", representing no index
    Attribute(string n, int t, bool i);

public:
    int static const TYPE_FLOAT = -1;
    int static const TYPE_INT = 0;
    string indexNameGet() {return index;}

    void print()
    {
        cout <<  "name: " << name << ";type: " << type << ";ifUnique: " << ifUnique << ";index: " << index << endl;
    }
};

#endif
