//
// Created by 钱诚亮 on 2018/5/12.
//


#include "../header/BufferManager.h"
#include "../header/Minisql.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <queue>

//构造器
BufferManager::BufferManager():total_block(0),total_file(0),fileHead(NULL)
{
    for (int i = 0; i < MAX_FILE_NUM; i ++)
    {
        file_pool[i].fileName = new char[MAX_FILE_NAME];
        if(file_pool[i].fileName == NULL)
        {
            printf("Can not allocate memory in initing the file pool!\n");
            exit (1);
        }
        init_file(file_pool[i]);
    }
    for (int i = 0; i < MAX_BLOCK_NUM; i ++) {
        block_pool[i].address = new char[BLOCK_SIZE];
        if(block_pool[i].address == NULL)
        {
            printf("Can not allocate memory in initing the block pool!\n");
            exit (1);
        }
        block_pool[i].fileName = new char[MAX_FILE_NAME];
        if(block_pool[i].fileName == NULL)
        {
            printf("Can not allocate memory in initing the block pool!\n");
            exit (1);
        }
        init_block(block_pool[i]);
    }
}

//析构器
BufferManager::~BufferManager()
{
    writtenBackToDiskAll();
    for (int i = 0; i < MAX_FILE_NUM; i ++)
    {
        delete [] file_pool[i].fileName;
    }
    for (int i = 0; i < MAX_BLOCK_NUM; i ++)
    {
        delete [] block_pool[i].address;
    }
}

//初始化filenode
void BufferManager::init_file(fileNode &file)
{
    file.nextFile = NULL;
    file.preFile = NULL;
    file.blockHead = NULL;
    file.pin = false;
    memset(file.fileName,0,MAX_FILE_NAME);
}

//初始化block
void BufferManager::init_block(blockNode &block)
{
    memset(block.address,0,BLOCK_SIZE);
    size_t init_usage = 0;
    memcpy(block.address, (char*)&init_usage, sizeof(size_t)); // set the block head
    block.using_size = sizeof(size_t);
    block.dirty = false;
    block.nextBlock = NULL;
    block.preBlock = NULL;
    block.offsetNum = -1;
    block.pin = false;
    block.reference = false;
    block.ifbottom = false;
    memset(block.fileName,0,MAX_FILE_NAME);
}

//获取一个fileNode
//如果文件已经在列表中了，那么返回fileNode
//如果文件不在列表中, 就替换

fileNode* BufferManager::getFile(const char * fileName, bool if_pin)
{
    blockNode * btmp = NULL;
    fileNode * ftmp = NULL;


//    cout << " fileName "<<fileName;

    if(fileHead != NULL)
    {
        for(ftmp = fileHead;ftmp != NULL;ftmp = ftmp->nextFile)
        {
            if(!strcmp(fileName, ftmp->fileName)) //the fileNode is already in the list
            {
                ftmp->pin = if_pin;
                return ftmp;
            }
        }
    }

    // 不在列表中
    if(total_file == 0) // 列表中没有文件
    {
        ftmp = &file_pool[total_file];
        total_file ++;
        fileHead = ftmp;
    }
    else if(total_file < MAX_FILE_NUM) // 有一些空的fileNode
    {
        ftmp = &file_pool[total_file];
        // 向列表尾部添加
        file_pool[total_file-1].nextFile = ftmp;
        ftmp->preFile = &file_pool[total_file-1];
        total_file ++;
    }
    else
    {
        ftmp = fileHead;
        while(ftmp->pin)
        {
            if(ftmp -> nextFile)ftmp = ftmp->nextFile;
            else
            {
                printf("There are no enough file node in the pool!");
                exit(2);
            }
        }
        for(btmp = ftmp->blockHead;btmp != NULL;btmp = btmp->nextBlock)
        {
            if(btmp->preBlock)
            {
                init_block(*(btmp->preBlock));
                total_block --;
            }
            writtenBackToDisk(btmp->fileName,btmp);
        }
        init_file(*ftmp);
    }
    if(strlen(fileName) + 1 > MAX_FILE_NAME)
    {
        printf("文件名长度过长，最高不能超过%d\n",MAX_FILE_NAME);
        exit(3);
    }
    strncpy(ftmp->fileName, fileName,MAX_FILE_NAME);
    set_pin(*ftmp, if_pin);
    return ftmp;
}

//获取一个fileNode
//如果文件已经在列表中了，那么返回fileNode
//如果文件不在列表中, 就替换

blockNode* BufferManager::getBlock(fileNode * file,blockNode *position, bool if_pin)
{
    const char * fileName = file->fileName;
    blockNode * btmp = NULL;
    if(total_block == 0)
    {
        btmp = &block_pool[0];
        total_block ++;
    }
    else if(total_block < MAX_BLOCK_NUM)
    {
        for(int i = 0 ;i < MAX_BLOCK_NUM;i ++)
        {
            if(block_pool[i].offsetNum == -1)
            {
                btmp = &block_pool[i];
                total_block ++;
                break;
            }
            else
                continue;
        }
    }
    else
    {
        int i = replaced_block;
        while (true)
        {
            i ++;
            if(i >= total_block) i = 0;
            if(!block_pool[i].pin)
            {
                if(block_pool[i].reference == true)
                    block_pool[i].reference = false;
                else
                {
                    btmp = &block_pool[i];
                    if(btmp->nextBlock) btmp -> nextBlock -> preBlock = btmp -> preBlock;
                    if(btmp->preBlock) btmp -> preBlock -> nextBlock = btmp -> nextBlock;
                    if(file->blockHead == btmp) file->blockHead = btmp->nextBlock;
                    replaced_block = i;

                    writtenBackToDisk(btmp->fileName, btmp);
                    init_block(*btmp);
                    break;
                }
            }
            else
                continue;
        }
    }

    if(position != NULL && position->nextBlock == NULL)
    {
        btmp -> preBlock = position;
        position -> nextBlock = btmp;
        btmp -> offsetNum = position -> offsetNum + 1;
    }
    else if (position !=NULL && position->nextBlock != NULL)
    {
        btmp->preBlock=position;
        btmp->nextBlock=position->nextBlock;
        position->nextBlock->preBlock=btmp;
        position->nextBlock=btmp;
        btmp -> offsetNum = position -> offsetNum + 1;
    }
    else // 当前block将作为列表头
    {
        btmp -> offsetNum = 0;
        if(file->blockHead) //如果文件的blockhead不正确
        {
            file->blockHead -> preBlock = btmp;
            btmp->nextBlock = file->blockHead;
        }
        file->blockHead = btmp;
    }
    set_pin(*btmp, if_pin);
    if(strlen(fileName) + 1 > MAX_FILE_NAME)
    {
        printf("文件名长度过长，最高不能超过%d\n",MAX_FILE_NAME);
        exit(3);
    }
    strncpy(btmp->fileName, fileName, MAX_FILE_NAME);


    FILE * fileHandle;
    if((fileHandle = fopen(fileName, "ab+")) != NULL)
    {
        if(fseek(fileHandle, btmp->offsetNum*BLOCK_SIZE, 0) == 0)
        {
            if(fread(btmp->address, 1, BLOCK_SIZE, fileHandle)==0)
                btmp->ifbottom = true;
            btmp ->using_size = getUsingSize(btmp);
        }
        else
        {
            printf("Problem seeking the file %s in reading",fileName);
            exit(1);
        }
        fclose(fileHandle);
    }
    else
    {
        printf("Problem opening the file %s in reading",fileName);
        exit(1);
    }
    return btmp;
}

//将block存到磁盘
//const char*  the file name
//blockNode&  the block your want to flush


void BufferManager::writtenBackToDisk(const char* fileName,blockNode* block)
{
    if(!block->dirty) // 当前block已经被修改, 所以不需要存回文件
    {
        return;
    }
    else //存回文件
    {
        FILE * fileHandle = NULL;
        if((fileHandle = fopen(fileName, "rb+")) != NULL)
        {
            if(fseek(fileHandle, block->offsetNum*BLOCK_SIZE, 0) == 0)
            {
                if(fwrite(block->address, block->using_size+sizeof(size_t), 1, fileHandle) != 1)
                {
                    printf("Problem writing the file %s in writtenBackToDisking",fileName);
                    exit(1);
                }
            }
            else
            {
                printf("Problem seeking the file %s in writtenBackToDisking",fileName);
                exit(1);
            }
            fclose(fileHandle);
        }
        else
        {
            printf("Problem opening the file %s in writtenBackToDisking",fileName);
            exit(1);
        }
    }
}

//将列表中的所有blockNode存到磁盘

void BufferManager::writtenBackToDiskAll()
{
    blockNode *btmp = NULL;
    fileNode *ftmp = NULL;
    if(fileHead)
    {
        for(ftmp = fileHead;ftmp != NULL;ftmp = ftmp ->nextFile)
        {
            if(ftmp->blockHead)
            {
                for(btmp = ftmp->blockHead;btmp != NULL;btmp = btmp->nextBlock)
                {
                    if(btmp->preBlock)init_block(*(btmp -> preBlock));
                    writtenBackToDisk(btmp->fileName, btmp);
                }
            }
        }
    }
}

//返回输入节点的下一个block节点
//fileNode* the file you want to add a block
// blockNode&  the block your want to be added to

blockNode* BufferManager::getNextBlock(fileNode* file,blockNode* block)
{
    if(block->nextBlock == NULL)
    {
        if(block->ifbottom) block->ifbottom = false;
        return getBlock(file, block);
    }
    else //block->nextBlock != NULL
    {
        if(block->offsetNum == block->nextBlock->offsetNum - 1)
        {
            return block->nextBlock;
        }
        else //the block list is not in the right order
        {
            return getBlock(file, block);
        }
    }
}

//获取文件的头block

blockNode* BufferManager::getBlockHead(fileNode* file)
{
    blockNode* btmp = NULL;
    if(file->blockHead != NULL)
    {
        if(file->blockHead->offsetNum == 0)
        {
            btmp = file->blockHead;
        }
        else
        {
            btmp = getBlock(file, NULL);
        }
    }
    else//如果当前文件没有block头，那么新建一个

    {
        btmp = getBlock(file,NULL);
    }
    return btmp;
}

//获取文件的block，利用offset number

blockNode* BufferManager::getBlockByOffset(fileNode* file, int offsetNumber)
{
    blockNode* btmp = NULL;
    if(offsetNumber == 0) return getBlockHead(file);
    else
    {
        btmp = getBlockHead(file);
        while( offsetNumber > 0)
        {
            btmp = getNextBlock(file, btmp);
            offsetNumber --;
        }
        return btmp;
    }
}

//删除文件节点和block节点

void BufferManager::delete_fileNode(const char * fileName)
{
    fileNode* ftmp = getFile(fileName);
    blockNode* btmp = getBlockHead(ftmp);
    queue<blockNode*> blockQ;
    while (true) {
        if(btmp == NULL) return;
        blockQ.push(btmp);
        if(btmp->ifbottom) break;
        btmp = getNextBlock(ftmp,btmp);
    }
    total_block -= blockQ.size();
    while(!blockQ.empty())
    {
        init_block(*blockQ.back());
        blockQ.pop();
    }
    if(ftmp->preFile) ftmp->preFile->nextFile = ftmp->nextFile;
    if(ftmp->nextFile) ftmp->nextFile->preFile = ftmp->preFile;
    if(fileHead == ftmp) fileHead = ftmp->nextFile;
    init_file(*ftmp);
    total_file --;
}


void BufferManager::set_pin(blockNode &block,bool pin)
{
    block.pin = pin;
    if(!pin)
        block.reference = true;
}

void BufferManager::set_pin(fileNode &file,bool pin)
{
    file.pin = pin;
}

//将block设置为dirty（已经被修改过的）
//blockNode&  the block your want to be added modify

void BufferManager::set_dirty(blockNode &block)
{
    block.dirty = true;
}

void BufferManager::clean_dirty(blockNode &block)
{
    block.dirty = false;
}


size_t BufferManager::getUsingSize(blockNode* block)
{
    return *(size_t*)block->address;
}

void BufferManager::set_usingSize(blockNode & block,size_t usage)
{
    block.using_size = usage;
    memcpy(block.address,(char*)&usage,sizeof(size_t));
}

size_t BufferManager::get_usingSize(blockNode & block)
{
    return block.using_size;

}

//获取block的内容（除了block头）

char* BufferManager::get_content(blockNode& block)
{
    return block.address + sizeof(size_t);

}





