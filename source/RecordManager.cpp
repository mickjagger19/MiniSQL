//
// Created by 钱诚亮 on 2018/5/12.
//


#include <iostream>
#include <cstring>
#include "../header/API.h"
#include "../header/RecordManager.h"


//建立表
//create table

int RecordManager::tableCreate(string tableName)
{
    //存放 table数据 文件名
    string tableFileName = tableFileNameGet(tableName);

    FILE *fp;
    fp = fopen(tableFileName.c_str(), "w+");
    if (fp == NULL)
    {
        return 0;
    }
    fclose(fp);
    return 1;
}

//删除表
//drop table
int RecordManager::tableDrop(string tableName)
{
    string tableFileName = tableFileNameGet(tableName);
    bm.delete_fileNode(tableFileName.c_str());
    if (remove(tableFileName.c_str()))
    {
        return 0;
    }
    return 1;
}

//建立索引
///create index
int RecordManager::indexCreate(string indexName)
{
    string indexFileName = indexFileNameGet(indexName);

    FILE *fp;
    fp = fopen(indexFileName.c_str(), "w+");
    if (fp == NULL)
    {
        return 0;
    }
    fclose(fp);
    return 1;
}

//删除索引
//drop index

int RecordManager::indexDrop(string indexName)
{
    string indexFileName = indexFileNameGet(indexName);
    bm.delete_fileNode(indexFileName.c_str());
    if (remove(indexFileName.c_str()))
    {
        return 0;
    }
    return 1;
}

//插入记录
//insert record

int RecordManager::recordInsert(string tableName,char* record, int recordSize)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode *btmp = bm.GetBlockHeader(ftmp);
    while (true)
    {
        if (btmp == NULL)
        {
            return -1;
        }
        if (bm.GetUsedSize(*btmp) <= bm.getBlockSize() - recordSize)
        {

            char* addressBegin;
            addressBegin = bm.GetContent(*btmp) + bm.GetUsedSize(*btmp);
            memcpy(addressBegin, record, recordSize);
            bm.SetUsedSize(*btmp, bm.GetUsedSize(*btmp) + recordSize);
            bm.SetModified(*btmp);
            return btmp->offsetNum;
        }
        else
        {
            btmp = bm.getNextBlock(ftmp, btmp);
        }
    }

    return -1;
}

//打印表中所有满足条件的记录
//tableName: name of table
//attributeNameVector: the attribute list
//conditionVector: the conditions list

int RecordManager::recordAllShow(string tableName, vector<string>* attributeNameVector,  vector<Condition>* conditionVector)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode *btmp = bm.GetBlockHeader(ftmp);
    int count = 0;
    while (true)
    {
        if (btmp == NULL)
        {
            return -1;
        }
        if (btmp->ifbottom)
        {
            int recordBlockNum = recordBlockShow(tableName,attributeNameVector, conditionVector, btmp);
            count += recordBlockNum;
            return count;
        }
        else
        {
            int recordBlockNum = recordBlockShow(tableName, attributeNameVector, conditionVector, btmp);
            count += recordBlockNum;
            btmp = bm.getNextBlock(ftmp, btmp);
        }
    }

    return -1;
}

//打印blaock中table中的记录
//tableName: name of table
//attributeNameVector: the attribute list
//onditionVector: the conditions list
//blockOffset: the block's offsetNum

int RecordManager::recordBlockShow(string tableName, vector<string>* attributeNameVector, vector<Condition>* conditionVector, int blockOffset)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode* block = bm.getBlockByOffset(ftmp, blockOffset);
    if (block == NULL)
    {
        return -1;
    }
    else
    {
        return  recordBlockShow(tableName, attributeNameVector, conditionVector, block);
    }
}

//打印blaock中table中的记录

int RecordManager::recordBlockShow(string tableName, vector<string>* attributeNameVector, vector<Condition>* conditionVector, blockNode* block)
{

    //if block is null, return -1
    if (block == NULL)
    {
        return -1;
    }

    int count = 0;

    char* recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int recordSize = api->recordSizeGet(tableName);

    api->attributeGet(tableName, &attributeVector);
    char* blockBegin = bm.GetContent(*block);
    size_t usingSize = bm.GetUsedSize(*block);

    while (recordBegin - blockBegin  < usingSize)
    {
        //如果recordBegin指向一条记录

        if(recordConditionFit(recordBegin, recordSize, &attributeVector, conditionVector))
        {
            count ++;
            recordPrint(recordBegin, recordSize, &attributeVector, attributeNameVector);
            printf("\n");
        }

        recordBegin += recordSize;
    }



    return count;
}

//寻找表中所有满足条件的记录数
//tableName: name of table
//conditionVector: the conditions list

int RecordManager::recordAllFind(string tableName, vector<Condition>* conditionVector)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode *btmp = bm.GetBlockHeader(ftmp);
    int count = 0;
    while (true)
    {
        if (btmp == NULL)
        {
            return -1;
        }
        if (btmp->ifbottom)
        {
            int recordBlockNum = recordBlockFind(tableName, conditionVector, btmp);
            count += recordBlockNum;
            return count;
        }
        else
        {
            int recordBlockNum = recordBlockFind(tableName, conditionVector, btmp);
            count += recordBlockNum;
            btmp = bm.getNextBlock(ftmp, btmp);
        }
    }

    return -1;
}

//寻找一个block的一个表中的记录数
//tableName: name of table
//block: search record in the block
//conditionVector: the conditions list

int RecordManager::recordBlockFind(string tableName, vector<Condition>* conditionVector, blockNode* block)
{
    //if block is null, return -1
    if (block == NULL)
    {
        return -1;
    }
    int count = 0;

    char* recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int recordSize = api->recordSizeGet(tableName);

    api->attributeGet(tableName, &attributeVector);

    while (recordBegin - bm.GetContent(*block)  < bm.GetUsedSize(*block))
    {


        if(recordConditionFit(recordBegin, recordSize, &attributeVector, conditionVector))
        {
            count++;
        }

        recordBegin += recordSize;

    }

    return count;
}

//删除表中所有满足条件的记录
//tableName: name of table
//onditionVector: the conditions list

int RecordManager::recordAllDelete(string tableName, vector<Condition>* conditionVector)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode *btmp = bm.GetBlockHeader(ftmp);

    int count = 0;
    while (true)
    {
        if (btmp == NULL)
        {
            return -1;
        }
        if (btmp->ifbottom)
        {
            int recordBlockNum = recordBlockDelete(tableName, conditionVector, btmp);
            count += recordBlockNum;
            return count;
        }
        else
        {
            int recordBlockNum = recordBlockDelete(tableName, conditionVector, btmp);
            count += recordBlockNum;
            btmp = bm.getNextBlock(ftmp, btmp);
        }
    }

    return -1;
}

//删除block中一个表的记录
//tableName: name of table
//conditionVector: the conditions list
//blockOffset: the block's offsetNum

int RecordManager::recordBlockDelete(string tableName,  vector<Condition>* conditionVector, int blockOffset)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode* block = bm.getBlockByOffset(ftmp, blockOffset);
    if (block == NULL)
    {
        return -1;
    }
    else
    {
        return  recordBlockDelete(tableName, conditionVector, block);
    }
}

//删除block中一个表的记录

int RecordManager::recordBlockDelete(string tableName,  vector<Condition>* conditionVector, blockNode* block)
{
    //if block is null, return -1
    if (block == NULL)
    {
        return -1;
    }
    int count = 0;

    char* recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int recordSize = api->recordSizeGet(tableName);

    api->attributeGet(tableName, &attributeVector);

    while (recordBegin - bm.GetContent(*block) < bm.GetUsedSize(*block))
    {


        if(recordConditionFit(recordBegin, recordSize, &attributeVector, conditionVector))
        {
            count ++;

            api->recordIndexDelete(recordBegin, recordSize, &attributeVector, block->offsetNum);
            int i = 0;
            for (i = 0; i + recordSize + recordBegin - bm.GetContent(*block) < bm.GetUsedSize(*block); i++)
            {
                recordBegin[i] = recordBegin[i + recordSize];
            }
            memset(recordBegin + i, 0, recordSize);
            bm.SetUsedSize(*block, bm.GetUsedSize(*block) - recordSize);
            bm.SetModified(*block);
        }
        else
        {
            recordBegin += recordSize;
        }
    }

    return count;
}

//添加一个表中所有记录的index
//tableName: name of table
//indexName: name of index

int RecordManager::indexRecordAllAlreadyInsert(string tableName,string indexName)
{
    fileNode *ftmp = bm.getFile(tableFileNameGet(tableName).c_str());
    blockNode *btmp = bm.GetBlockHeader(ftmp);
    int count = 0;
    while (true)
    {
        if (btmp == NULL)
        {
            return -1;
        }
        if (btmp->ifbottom)
        {
            int recordBlockNum = indexRecordBlockAlreadyInsert(tableName, indexName, btmp);
            count += recordBlockNum;
            return count;
        }
        else
        {
            int recordBlockNum = indexRecordBlockAlreadyInsert(tableName, indexName, btmp);
            count += recordBlockNum;
            btmp = bm.getNextBlock(ftmp, btmp);
        }
    }

    return -1;
}


//向block中添加某条记录的index

int RecordManager::indexRecordBlockAlreadyInsert(string tableName,string indexName,  blockNode* block)
{
    //if block is null, return -1
    if (block == NULL)
    {
        return -1;
    }
    int count = 0;

    char* recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int recordSize = api->recordSizeGet(tableName);

    api->attributeGet(tableName, &attributeVector);

    int type;
    int typeSize;
    char * contentBegin;

    while (recordBegin - bm.GetContent(*block)  < bm.GetUsedSize(*block))
    {
        contentBegin = recordBegin;
        //if the recordBegin point to a record
        for (int i = 0; i < attributeVector.size(); i++)
        {
            type = attributeVector[i].type;
            typeSize = api->typeSizeGet(type);

            //find the index  of the record, and insert it to index tree
            if (attributeVector[i].index == indexName)
            {
                api->indexInsert(indexName, contentBegin, type, block->offsetNum);
                count++;
            }

            contentBegin += typeSize;
        }
        recordBegin += recordSize;
    }

    return count;
}

//判断记录是否复合条件

bool RecordManager::recordConditionFit(char* recordBegin,int recordSize, vector<Attribute>* attributeVector,vector<Condition>* conditionVector)
{
    if (conditionVector == NULL) {
        return true;
    }
    int type;
    string attributeName;
    int typeSize;
    char content[255];

    char *contentBegin = recordBegin;
    for(int i = 0; i < attributeVector->size(); i++)
    {
        type = (*attributeVector)[i].type;
        attributeName = (*attributeVector)[i].name;
        typeSize = api->typeSizeGet(type);

        //init content (when content is string , we can get a string easily)
        memset(content, 0, 255);
        memcpy(content, contentBegin, typeSize);
        for(int j = 0; j < (*conditionVector).size(); j++)
        {
            if ((*conditionVector)[j].attributeName == attributeName)
            {
                //if this attribute need to deal about the condition
                if(!contentConditionFit(content, type, &(*conditionVector)[j]))
                {
                    //if this record is not fit the conditon
                    return false;
                }
            }
        }

        contentBegin += typeSize;
    }
    return true;
}

//打印记录的值
//recordBegin: point to a record
//recordSize: size of the record
//attributeVector: the attribute list of the record
//attributeVector: the name list of all attribute you want to print

void RecordManager::recordPrint(char* recordBegin, int recordSize, vector<Attribute>* attributeVector, vector<string> *attributeNameVector)
{
    int type;
    string attributeName;
    int typeSize;
    char content[255];

    char *contentBegin = recordBegin;
    for(int i = 0; i < attributeVector->size(); i++)
    {
        cout<<"| ";
        type = (*attributeVector)[i].type;
        typeSize = api->typeSizeGet(type);

        //init content (when content is string , we can get a string easily)
        memset(content, 0, 255);

        memcpy(content, contentBegin, typeSize);

        for(int j = 0; j < (*attributeNameVector).size(); j++)
        {

            if ((*attributeNameVector)[j] == (*attributeVector)[i].name) {
                contentPrint(content, type);
                int length=0;


                if (type==0) {
                    int tmp = *((int *) content);
                    if (tmp>1000) length=4;
                    else if (tmp>100) length=3;
                    else if (tmp>10) length=2;
                    else if (tmp>1) length=1;
                }
                else {
                    while (content[length++] != '\0') {};
                    length--;
                }
//                cout << " contentbegin " << *contentBegin;
//                cout << " content " << *content;
//                cout << " recordbegin " << *recordBegin ;

                int k;
                length=(*attributeVector)[i].name.length()*4-2-length;
                for (k = 0 ; k <= length ; k++) {
                    cout << " ";
                }
                break;
            }

        }




        contentBegin += typeSize;

    }
    cout<<"|";


}

//打印元组值

void RecordManager::contentPrint(char * content, int type)
{
    if (type == Attribute::TYPE_INT)
    {
        //if the content is a int
        int tmp = *((int *) content);   //get content value by point
        printf("%d ", tmp);
    }
    else if (type == Attribute::TYPE_FLOAT)
    {
        //if the content is a float
        float tmp = *((float *) content);   //get content value by point
        printf("%f ", tmp);
    }
    else
    {
        //if the content is a string
        string tmp = content;
        printf("%s ", tmp.c_str());
    }

}

//判断内容是否符合条件
//content: point to content
//type: type of content
//condition: condition

bool RecordManager::contentConditionFit(char* content,int type,Condition* condition)
{
    if (type == Attribute::TYPE_INT)
    {

        int tmp = *((int *) content);
        return condition->ifRight(tmp);
    }
    else if (type == Attribute::TYPE_FLOAT)
    {

        float tmp = *((float *) content);
        return condition->ifRight(tmp);
    }
    else
    {

        return condition->ifRight(content);
    }
    return true;
}

//获取index的文件名

string RecordManager::indexFileNameGet(string indexName)
{
    string tmp = "";
    return "INDEX_FILE_"+indexName;
}

//返回一张表的文件名
string RecordManager::tableFileNameGet(string tableName)
{
    string tmp = "";
    return  tmp + "TABLE_FILE_" + tableName;
}
