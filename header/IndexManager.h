//
// Created by 钱诚亮 on 2018/5/12.
//


#ifndef __Minisql__IndexManager__
#define __Minisql__IndexManager__

#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include "Attribute.h"
#include "BPlusTree.h"

class API;

class IndexManager{
private:

    typedef map<string,BPlusTree<int> *> intMap;
    typedef map<string,BPlusTree<string> *> stringMap;
    typedef map<string,BPlusTree<float> *> floatMap;

    int static const TYPE_FLOAT = Attribute::TYPE_FLOAT;
    int static const TYPE_INT = Attribute::TYPE_INT;
    //其他值的意思是char的尺寸

    API *api; // to call the functions of API

    intMap indexIntMap;
    stringMap indexStringMap;
    floatMap indexFloatMap;
    struct keyTmp{
        int intTmp;
        float floatTmp;
        string stringTmp;
    };
    // KeyTmp用于把字符串转换成相应的值
    struct keyTmp KeySet;

    int getDegree(int type);

    int getKeySize(int type);

    void SetKey(int type,string key);


public:
    IndexManager(API *api);
    ~IndexManager();


    void createIndex(string filePath,int type);

    void dropIndex(string filePath,int type);

    offsetNumber searchIndex(string filePath,string key,int type);

    void insertIndex(string filePath,string key,offsetNumber blockOffset,int type);

    void deleteIndexByKey(string filePath,string key,int type);
};


#endif
