//
// Created by 钱诚亮 on 2018/5/12.
//

#include <vector>
#include <iostream>
#include "../header/IndexManager.h"
#include "../header/API.h"
#include "../header/IndexInfo.h"



using namespace std;


//构造器

IndexManager::IndexManager(API *m_api)
{
    api = m_api;
    vector<IndexInfo> allIndexInfo;
    api->allIndexAddressInfoGet(&allIndexInfo);
    for(vector<IndexInfo>::iterator i = allIndexInfo.begin();i != allIndexInfo.end();i ++)
    {
        createIndex(i->indexName, i->type);
    }
}


//析构器

IndexManager::~IndexManager()
{

    for(intMap::iterator itInt = indexIntMap.begin();itInt != indexIntMap.end(); itInt ++)
    {
        if(itInt->second)
        {
            itInt -> second->WriteAlltoDisk();
            delete itInt->second;
        }
    }
    for(stringMap::iterator itString = indexStringMap.begin();itString != indexStringMap.end(); itString ++)
    {
        if(itString->second)
        {
            itString ->second-> WriteAlltoDisk();
            delete itString->second;
        }
    }
    for(floatMap::iterator itFloat = indexFloatMap.begin();itFloat != indexFloatMap.end(); itFloat ++)
    {
        if(itFloat->second)
        {
            itFloat ->second-> WriteAlltoDisk();
            delete itFloat->second;
        }
    }
}


//根据指定类型建立index索引

void IndexManager::createIndex(string filePath,int type)
{
    int keySize = getKeySize(type);
    int degree = getDegree(type);
    if(type == TYPE_INT)
    {
        BPlusTree<int> *tree = new BPlusTree<int>(filePath,keySize,degree);
        indexIntMap.insert(intMap::value_type(filePath, tree));
    }
    else if(type == TYPE_FLOAT)
    {
        BPlusTree<float> *tree = new BPlusTree<float>(filePath,keySize,degree);
        indexFloatMap.insert(floatMap::value_type(filePath, tree));
    }
    else // string
    {
        BPlusTree<string> *tree = new BPlusTree<string>(filePath,keySize,degree);
        indexStringMap.insert(stringMap::value_type(filePath, tree));
    }

}

//删除指定index

void IndexManager::dropIndex(string filePath,int type)
{
    if(type == TYPE_INT)
    {
        intMap::iterator itInt = indexIntMap.find(filePath);
        if(itInt == indexIntMap.end())
        {
            cout << "Error:in drop index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            delete itInt->second;
            indexIntMap.erase(itInt);
        }
    }
    else if(type == TYPE_FLOAT)
    {
        floatMap::iterator itFloat = indexFloatMap.find(filePath);
        if(itFloat == indexFloatMap.end())
        {
            cout << "Error:in drop index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            delete itFloat->second;
            indexFloatMap.erase(itFloat);
        }
    }
    else
    {
        stringMap::iterator itString = indexStringMap.find(filePath);
        if(itString == indexStringMap.end())
        {
            cout << "Error:in drop index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            delete itString->second;
            indexStringMap.erase(itString);
        }
    }

}

//在B+树中根据key搜索满足条件的记录

offsetNumber IndexManager::searchIndex(string filePath,string key,int type)
{
    SetKey(type, key);

    //如果是整型
    if(type == TYPE_INT)
    {
        //新建迭代器
        intMap::iterator itInt = indexIntMap.find(filePath);
        if(itInt == indexIntMap.end())
        //如果没找到
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return -1;
        }
        else
        {
            return itInt->second->search(KeySet.intTmp);
        }
    }
    //如果是浮点型
    else if(type == TYPE_FLOAT)
    {
        floatMap::iterator itFloat = indexFloatMap.find(filePath);
        if(itFloat == indexFloatMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return -1;
        }
        else
        {
            return itFloat->second->search(KeySet.floatTmp);

        }
    }
    //如果是字符串
    else
    {
        stringMap::iterator itString = indexStringMap.find(filePath);
        if(itString == indexStringMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return -1;
        }
        else
        {
            return itString->second->search(key);
        }
    }
}

//向B+树插入值，同上

void IndexManager::insertIndex(string filePath,string key,offsetNumber blockOffset,int type)
{
    SetKey(type, key);

    if(type == TYPE_INT)
    {
        intMap::iterator itInt = indexIntMap.find(filePath);
        if(itInt == indexIntMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            itInt->second->InsertKey(KeySet.intTmp,blockOffset);
        }
    }
    else if(type == TYPE_FLOAT)
    {
        floatMap::iterator itFloat = indexFloatMap.find(filePath);
        if(itFloat == indexFloatMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            itFloat->second->InsertKey(KeySet.floatTmp,blockOffset);

        }
    }
    else // string
    {
        stringMap::iterator itString = indexStringMap.find(filePath);
        if(itString == indexStringMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            itString->second->InsertKey(key,blockOffset);
        }
    }
}

//删除值，大致同上

void IndexManager::deleteIndexByKey(string filePath,string key,int type)
{
    SetKey(type, key);

    if(type == TYPE_INT)
    {
        intMap::iterator itInt = indexIntMap.find(filePath);
        if(itInt == indexIntMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            itInt->second->deleteKey(KeySet.intTmp);
        }
    }
    else if(type == TYPE_FLOAT)
    {
        floatMap::iterator itFloat = indexFloatMap.find(filePath);
        if(itFloat == indexFloatMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            itFloat->second->deleteKey(KeySet.floatTmp);

        }
    }
    else // string
    {
        stringMap::iterator itString = indexStringMap.find(filePath);
        if(itString == indexStringMap.end())
        {
            cout << "Error:in search index, no index " << filePath <<" exits" << endl;
            return;
        }
        else
        {
            itString->second->deleteKey(key);
        }
    }
}

//根据类型获取degree

int IndexManager::getDegree(int type)
{
    int degree = bm.getBlockSize()/(getKeySize(type)+sizeof(offsetNumber));
    if(degree %2 == 0) degree -= 1;
    return degree;
}

//根据类型获取值的空间大小

int IndexManager::getKeySize(int type)
{
    if(type == TYPE_FLOAT)
        return sizeof(float);
    else if(type == TYPE_INT)
        return sizeof(int);
    else if(type > 0)
        return type + 1;
    else
    {
        cout << "ERROR: in getKeySize: invalid type" << endl;
        return -100;
    }
}

//根据输入字符串和类型获取值

void IndexManager::SetKey(int type,string key)
{
    stringstream ss;
    ss << key;
    if(type == this->TYPE_INT)
        ss >> this->KeySet.intTmp;
    else if(type == this->TYPE_FLOAT)
        ss >> this->KeySet.floatTmp;
    else if(type > 0)
        ss >> this->KeySet.stringTmp;
    else
        cout << "Error: in getKey: invalid type" << endl;

}






