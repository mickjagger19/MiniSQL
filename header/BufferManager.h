//
// Created by 钱诚亮 on 2018/5/12.
//



#ifndef __Minisql__BufferManager__
#define __Minisql__BufferManager__
#include "Minisql.h"
#include <stdio.h>

static int replaced_block = -1;


class BufferManager
{
private:
    fileNode *fileHead;
    fileNode file_pool[MAX_FILE_NUM];
    blockNode block_pool[MAX_BLOCK_NUM];
    int total_block; // the number of block that have been used, which means the block is in the list.
    int total_file; // the number of file that have been used, which means the file is in the list.
    void init_block(blockNode & block);
    void init_file(fileNode & file);
    blockNode* getBlock(fileNode * file,blockNode* position,bool if_pin = false);
    void WriteAlltoDisk();
    void writtenBackToDisk(const char* fileName,blockNode* block);
    void clean_Modified(blockNode &block);
    size_t getUsingSize(blockNode* block);
    static const int BLOCK_SIZE = 4096;

public:
    BufferManager();
    ~BufferManager();
    void delete_fileNode(const char * fileName);
    fileNode* getFile(const char* fileName,bool if_pin = false);
    void SetModified(blockNode & block);
    void set_pin(blockNode & block,bool pin);
    void set_pin(fileNode & file,bool pin);
    void SetUsedSize(blockNode & block,size_t usage);
    size_t get_usingSize(blockNode & block);
    char* get_content(blockNode& block);
    static int getBlockSize() //Get the size of the block that others can use.Others cannot use the block head
    {
        return BLOCK_SIZE - sizeof(size_t);
    }


    blockNode* getNextBlock(fileNode * file,blockNode* block);
    blockNode* GetBlockHeader(fileNode* file);
    blockNode* getBlockByOffset(fileNode* file, int offestNumber);

};

#endif /* defined(__Minisql__BufferManager__) */


