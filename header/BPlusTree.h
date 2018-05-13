//
// Created by 钱诚亮 on 2018/5/12.
//

//#ifndef MINISQL_B_TREE_H
//#define MINISQL_B_TREE_H
//
//#endif //MINISQL_B_TREE_H
//



#ifndef __Minisql__BPlusTree__
#define __Minisql__BPlusTree__
#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>
#include "BufferManager.h"
#include "Minisql.h"
using namespace std;

static BufferManager bm;
//**********************TreeNode***************************//
typedef int offsetNumber; // the value of the tree node

template <typename KeyType>
class TreeNode{
public:
    size_t count;  // the count of keys
    TreeNode* parent;
    vector <KeyType> keys;
    vector <TreeNode*> childs;
    vector <offsetNumber> vals;

    TreeNode* nextLeafNode; // point to the next leaf node

    bool isLeaf; // the flag whether this node is a leaf node

private:
    int degree;

public:
    //create a new node. if the newLeaf = false, create a branch node.Otherwise, create a leaf node
    TreeNode(int degree,bool newLeaf=false);
    ~TreeNode();

public:
    bool isRoot();
    bool search(KeyType key,size_t &index);//search a key and return by the reference of a parameter
    TreeNode* splite(KeyType &key);
    size_t add(KeyType &key); //add the key in the branch and return the position
    size_t add(KeyType &key,offsetNumber val); // add a key-value in the leaf node and return the position
    bool removeAt(size_t index);

#ifdef _DEBUG
    public:
    void debug_print();
#endif
};


//**********************BplusTree***************************//

template <typename KeyType>
class BPlusTree
{
private:
    typedef TreeNode<KeyType>* Node;


    struct searchNodeParse
    {
        Node pNode;
        size_t index; // 值的位置
        bool ifFound;
    };
private:
    string fileName;
    Node root;
    Node leafHead; // 叶子节点的头
    size_t keyCount;
    size_t level;
    size_t nodeCount;
    fileNode* file;
    int keySize;
    int degree;

public:
    BPlusTree(string m_name,int keySize,int degree);
    ~BPlusTree();

    offsetNumber search(KeyType& key);
    bool insertKey(KeyType &key,offsetNumber val);
    bool deleteKey(KeyType &key);

    void dropTree(Node node);

    void readFromDiskAll();
    void writtenbackToDiskAll();
    void readFromDisk(blockNode* btmp);

private:
    void init_tree();
    bool adjustAfterinsert(Node pNode);
    bool adjustAfterDelete(Node pNode);
    void findToLeaf(Node pNode,KeyType key,searchNodeParse &snp);

//DEBUG
#ifdef _DEBUG
    public:
    void debug_print();

    void debug_print_node(Node pNode);
#endif

};




//B+树实现
//构造器

template <class KeyType>
TreeNode<KeyType>::TreeNode(int m_degree,bool newLeaf):count(0),parent(NULL),nextLeafNode(NULL),isLeaf(newLeaf),degree(m_degree)
{
    for(size_t i = 0;i < degree+1;i ++)
    {
        childs.push_back(NULL);
        keys.push_back(KeyType());
        vals.push_back(offsetNumber());
    }
    childs.push_back(NULL);
}

//析构
template <class KeyType>
TreeNode<KeyType>::~TreeNode()
{

}

//检查当前节点是不是根

template <class KeyType>
bool TreeNode<KeyType>::isRoot()
{
    if(parent != NULL) return false;
    else return true;
}

//搜索节点中的值，返回该值是否在节点中
//size_t the position of the node by reference

template <class KeyType>
bool TreeNode<KeyType>::search(KeyType key,size_t &index)
{
    if(count == 0 ) // no values in the node
    {
        index = 0;
        return false;
    }
    else
    {
        // test if key are beyond the area of the keys array
        if(keys[count-1] < key)
        {
            index = count;
            return false;
        }
        else if(keys[0] > key)
        {
            index = 0;
            return false;
        } // end of test
        else if(count <= 20) // sequential search
        {
            for(size_t i = 0;i < count;i ++)
            {
                if(keys[i] == key)
                {
                    index = i;
                    return true;
                }
                else if(keys[i] < key)
                {
                    continue;
                }
                else if(keys[i] > key)
                {
                    index = i;
                    return false;
                }
            }
        } // end sequential search
        else if(count > 20) // too many keys, binary search. 2* log(n,2) < (1+n)/2
        {
            size_t left = 0, right = count - 1, pos = 0;
            while(right>left+1)
            {
                pos = (right + left) / 2;
                if(keys[pos] == key)
                {
                    index = pos;
                    return true;
                }
                else if(keys[pos] < key)
                {
                    left = pos;
                }
                else if(keys[pos] > key)
                {
                    right = pos;
                }
            } // end while

            // right == left + 1
            if(keys[left] >= key)
            {
                index = left;
                return (keys[left] == key);
            }
            else if(keys[right] >= key)
            {
                index = right;
                return (keys[right] == key);
            }
            else if(keys[right] < key)
            {
                index = right ++;
                return false;
            }
        } // end binary search
    }
    return false;
}

//把当前节点分成两个
//KeyType & the key reference returns the key that will go to the upper level
template <class KeyType>
TreeNode<KeyType>* TreeNode<KeyType>::splite(KeyType &key)
{
    size_t minmumNode = (degree - 1) / 2;
    TreeNode* newNode = new TreeNode(degree,this->isLeaf);
    if(newNode == NULL)
    {
        cout << "Problems in allocate momeory of TreeNode in splite node of " << key << endl;
        exit(2);
    }

    if(isLeaf) // 这是一个叶子
    {
        key = keys[minmumNode + 1];
        for(size_t i = minmumNode + 1;i < degree;i ++)
        {
            newNode->keys[i-minmumNode-1] = keys[i];
            keys[i] = KeyType();
            newNode->vals[i-minmumNode-1] = vals[i];
            vals[i] = offsetNumber();
        }
        newNode->nextLeafNode = this->nextLeafNode;
        this->nextLeafNode = newNode;

        newNode->parent = this->parent;
        newNode->count = minmumNode;
        this->count = minmumNode + 1;
    } // end leaf
    else if(!isLeaf)
    {
        key = keys[minmumNode];
        for(size_t i = minmumNode + 1;i < degree+1;i ++)
        {
            newNode->childs[i-minmumNode-1] = this->childs[i];
            newNode->childs[i-minmumNode-1]->parent = newNode;
            this->childs[i] = NULL;
        }
        for(size_t i = minmumNode + 1;i < degree;i ++)
        {
            newNode->keys[i-minmumNode-1] = this->keys[i];
            this->keys[i] = KeyType();
        }
        this->keys[minmumNode] = KeyType();
        newNode->parent = this->parent;
        newNode->count = minmumNode;
        this->count = minmumNode;
    }
    return newNode;
}

//向节点中添加值，返回添加的值的位置

template <class KeyType>
size_t TreeNode<KeyType>::add(KeyType &key)
{
    if(count == 0)
    {
        keys[0] = key;
        count ++;
        return 0;
    }
    else
    {
        size_t index = 0;
        bool exist = search(key, index);
        if(exist)
        {
            cout << "Error:In add(Keytype &key),key has already in the tree!" << endl;
            exit(3);
        }
        else
        {
            for(size_t i = count;i > index;i --)
                keys[i] = keys[i-1];
            keys[index] = key;

            for(size_t i = count + 1;i > index+1;i --)
                childs[i] = childs[i-1];
            childs[index+1] = NULL;
            count ++;

            return index;
        }
    }
}

//向叶子中添加值，返回添加的值的位置
//offsetNumber the value

template <class KeyType>
size_t TreeNode<KeyType>::add(KeyType &key,offsetNumber val)
{
    if(!isLeaf)
    {
        cout << "Error:add(KeyType &key,offsetNumber val) is a function for leaf nodes" << endl;
        return -1;
    }
    if(count == 0)
    {
        keys[0] = key;
        vals[0] = val;
        count ++;
        return 0;
    }
    else
    {
        size_t index = 0; // 记录树的位置
        bool exist = search(key, index);
        if(exist)
        {
            cout << "Error:In add(Keytype &key, offsetNumber val),key has already in the tree!" << endl;
            exit(3);
        }
        else
        {
            for(size_t i = count;i > index;i --)
            {
                keys[i] = keys[i-1];
                vals[i] = vals[i-1];
            }
            keys[index] = key;
            vals[index] = val;
            count ++;
            return index;
        }
    }
}

//删除键值或键的孩子
//size_t the position to delete

template <class KeyType>
bool TreeNode<KeyType>::removeAt(size_t index)
{
    if(index > count)
    {
        cout << "Error:In removeAt(size_t index), index is more than count!" << endl;
        return false;
    }
    else
    {
        if(isLeaf)
        {
            for(size_t i = index;i < count-1;i ++)
            {
                keys[i] = keys[i+1];
                vals[i] = vals[i+1];
            }
            keys[count-1] = KeyType();
            vals[count-1] = offsetNumber();
        }
        else
        {
            for(size_t i = index;i < count-1;i ++)
                keys[i] = keys[i+1];

            for(size_t i = index+1;i < count;i ++)
                childs[i] = childs[i+1];

            keys[count-1] = KeyType();
            childs[count] = NULL;
        }

        count --;
        return true;
    }
}

#ifdef _DEBUG
//打印整个树
template <class KeyType>
void TreeNode<KeyType>::debug_print()
{
    cout << "############DEBUG for node###############" << endl;

    cout << "Address: " << (void*)this << ",count: " << count << ",Parent: " << (void*)parent << ",isleaf: " << isLeaf << ",nextNode: " << (void*)nextLeafNode << endl;
    cout << "KEYS:{";
    for(size_t i = 0;i < count;i ++)
    {
        cout << keys[i] << " ";
    }
    cout << "}" << endl;
    if(isLeaf)
    {
        cout << "VALS:{";
        for(size_t i = 0;i < count;i ++)
        {
            cout << vals[i] << " ";
        }
        cout << "}" << endl;
    }
    else // nonleaf node
    {
        cout << "CHILDREN:{";
        for(size_t i = 0;i < count + 1;i ++)
        {
            cout << (void*)childs[i] << " ";
        }

        cout << "}" << endl;
    }
    cout << "#############END OF DEBUG IN NODE########"<< endl;
}
#endif

//******** B+树中的类型定义 **********


//构造器

template <class KeyType>
BPlusTree<KeyType>::BPlusTree(string m_name,int keysize,int m_degree):fileName(m_name),keyCount(0),level(0),nodeCount(0),root(NULL),leafHead(NULL),keySize(keysize),file(NULL),degree(m_degree)
{
    init_tree();
    readFromDiskAll();
}

//析构器
template <class KeyType>
BPlusTree<KeyType>:: ~BPlusTree()
{
    dropTree(root);
    keyCount = 0;
    root = NULL;
    level = 0;
}

//初始化树，分配节点的空间

template <class KeyType>
void BPlusTree<KeyType>::init_tree()
{
    root = new TreeNode<KeyType>(degree,true);
    keyCount = 0;
    level = 1;
    nodeCount = 1;
    leafHead = root;
}

//向叶子层搜索节点，以寻找包含值的节点

template <class KeyType>
void BPlusTree<KeyType>::findToLeaf(Node pNode,KeyType key,searchNodeParse & snp)
{
    size_t index = 0;
    if(pNode->search(key,index)) // 找节点中的值
    {
        if(pNode->isLeaf)
        {
            snp.pNode = pNode;
            snp.index = index;
            snp.ifFound = true;
        }
        else
        {
            pNode = pNode -> childs[index + 1];
            while(!pNode->isLeaf)
            {
                pNode = pNode->childs[0];
            }
            snp.pNode = pNode;
            snp.index = 0;
            snp.ifFound = true;
        }

    }
    else //找不到
    {
        if(pNode->isLeaf)
        {
            snp.pNode = pNode;
            snp.index = index;
            snp.ifFound = false;
        }
        else
        {
            findToLeaf(pNode->childs[index],key,snp);
        }
    }
}

//在合适的位置添加值，再调整结构

template <class KeyType>
bool BPlusTree<KeyType>::insertKey(KeyType &key,offsetNumber val)
{
    searchNodeParse snp;
    if(!root) init_tree();
    findToLeaf(root,key,snp);
    if(snp.ifFound)
    {
        cout << "Error:in insert key to index: the duplicated key!" << endl;
        return false;
    }
    else
    {
        snp.pNode->add(key,val);
        if(snp.pNode->count == degree)
        {
            adjustAfterinsert(snp.pNode);
        }
        keyCount ++;
        return true;
    }
}

//添加后调整节点，递归调用

template <class KeyType>
bool BPlusTree<KeyType>::adjustAfterinsert(Node pNode)
{
    KeyType key;
    Node newNode = pNode->splite(key);
    nodeCount ++;

    if(pNode->isRoot())
    {
        Node root = new TreeNode<KeyType>(degree,false);
        if(root == NULL)
        {
            cout << "Error: can not allocate memory for the new root in adjustAfterinsert" << endl;
            exit(1);
        }
        else
        {
            level ++;
            nodeCount ++;
            this->root = root;
            pNode->parent = root;
            newNode->parent = root;
            root->add(key);
            root->childs[0] = pNode;
            root->childs[1] = newNode;
            return true;
        }
    }
    else
        //如果不是节点
    {
        Node parent = pNode->parent;
        size_t index = parent->add(key);

        parent->childs[index+1] = newNode;
        newNode->parent = parent;
        if(parent->count == degree)
            return adjustAfterinsert(parent);

        return true;
    }
}

//搜索树以寻找指定键值

template <class KeyType>
offsetNumber BPlusTree<KeyType>::search(KeyType& key)
{
    if(!root) return -1;
    searchNodeParse snp;
    findToLeaf(root, key, snp);
    if(!snp.ifFound)
    {
        return -1; //没找到
    }
    else
    {
        return snp.pNode->vals[snp.index];
    }

}

//删除指定键值，再调整结构

template <class KeyType>
bool BPlusTree<KeyType>::deleteKey(KeyType &key)
{
    searchNodeParse snp;
    if(!root)
    {
        cout << "ERROR: In deleteKey, no nodes in the tree " << fileName << "!" << endl;
        return false;
    }
    else
    {
        findToLeaf(root, key, snp);
        if(!snp.ifFound)
        {
            cout << "ERROR: In deleteKey, no keys in the tree " << fileName << "!" << endl;
            return false;
        }
        else
        {
            if(snp.pNode->isRoot())
            {
                snp.pNode->removeAt(snp.index);
                keyCount --;
                return adjustAfterDelete(snp.pNode);
            }
            else
            {
                if(snp.index == 0 && leafHead != snp.pNode)
                {

                    size_t index = 0;

                    Node now_parent = snp.pNode->parent;
                    bool if_found_inBranch = now_parent->search(key,index);
                    while(!if_found_inBranch)
                    {
                        if(now_parent->parent)
                            now_parent = now_parent->parent;
                        else
                        {
                            break;
                        }
                        if_found_inBranch = now_parent->search(key,index);
                    }

                    now_parent -> keys[index] = snp.pNode->keys[1];

                    snp.pNode->removeAt(snp.index);
                    keyCount--;
                    return adjustAfterDelete(snp.pNode);

                }
                else
                {
                    snp.pNode->removeAt(snp.index);
                    keyCount--;
                    return adjustAfterDelete(snp.pNode);
                }
            }
        }
    }
}

//删除后调整结构，递归调用

template <class KeyType>
bool BPlusTree<KeyType>::adjustAfterDelete(Node pNode)
{
    size_t minmumKey = (degree - 1) / 2;
    if(((pNode->isLeaf)&&(pNode->count >= minmumKey)) || ((degree != 3)&&(!pNode->isLeaf)&&(pNode->count >= minmumKey - 1)) || ((degree ==3)&&(!pNode->isLeaf)&&(pNode->count < 0))) // do not need to adjust
    {
        return  true;
    }
    if(pNode->isRoot())
    {
        if(pNode->count > 0) //不需要调整
        {
            return true;
        }
        else
        {
            if(root->isLeaf) //根是叶子
            {
                delete pNode;
                root = NULL;
                leafHead = NULL;
                level --;
                nodeCount --;
            }
            else //根不是叶子
            {
                root = pNode -> childs[0];
                root -> parent = NULL;
                delete pNode;
                level --;
                nodeCount --;
            }
        }
    }
    else
    {
        Node parent = pNode->parent,brother = NULL;
        if(pNode->isLeaf)
        {
            size_t index = 0;
            parent->search(pNode->keys[0],index);

            if((parent->childs[0] != pNode) && (index + 1 == parent->count)) //向左兄弟合并
            {
                brother = parent->childs[index];
                if(brother->count > minmumKey) //选择左兄弟最大的键值并取出、合并
                {
                    for(size_t i = pNode->count;i > 0;i --)
                    {
                        pNode->keys[i] = pNode->keys[i-1];
                        pNode->vals[i] = pNode->vals[i-1];
                    }
                    pNode->keys[0] = brother->keys[brother->count-1];
                    pNode->vals[0] = brother->vals[brother->count-1];
                    brother->removeAt(brother->count-1);

                    pNode->count ++;
                    parent->keys[index] = pNode->keys[0];
                    return true;

                }
                else //合并
                {
                    parent->removeAt(index);

                    for(int i = 0;i < pNode->count;i ++)
                    {
                        brother->keys[i+brother->count] = pNode->keys[i];
                        brother->vals[i+brother->count] = pNode->vals[i];
                    }
                    brother->count += pNode->count;
                    brother->nextLeafNode = pNode->nextLeafNode;

                    delete pNode;
                    nodeCount --;

                    return adjustAfterDelete(parent);
                }

            }
            else //选择右兄弟
            {
                if(parent->childs[0] == pNode)
                    brother = parent->childs[1];
                else
                    brother = parent->childs[index+2];
                if(brother->count > minmumKey)//选择右兄弟最小的值并合并
                {
                    pNode->keys[pNode->count] = brother->keys[0];
                    pNode->vals[pNode->count] = brother->vals[0];
                    pNode->count ++;
                    brother->removeAt(0);
                    if(parent->childs[0] == pNode)
                        parent->keys[0] = brother->keys[0];
                    else
                        parent->keys[index+1] = brother->keys[0];
                    return true;

                }
                else // 向兄弟合并
                {
                    for(int i = 0;i < brother->count;i ++)
                    {
                        pNode->keys[pNode->count+i] = brother->keys[i];
                        pNode->vals[pNode->count+i] = brother->vals[i];
                    }
                    if(pNode == parent->childs[0])
                        parent->removeAt(0);
                    else
                        parent->removeAt(index+1);
                    pNode->count += brother->count;
                    pNode->nextLeafNode = brother->nextLeafNode;
                    delete brother;
                    nodeCount --;

                    return adjustAfterDelete(parent);
                }
            }

        }
        else
        {
            size_t index = 0;
            parent->search(pNode->childs[0]->keys[0],index);
            if((parent->childs[0] != pNode) && (index + 1 == parent->count))
            {
                brother = parent->childs[index];
                if(brother->count > minmumKey - 1)
                {

                    pNode->childs[pNode->count+1] = pNode->childs[pNode->count];
                    for(size_t i = pNode->count;i > 0;i --)
                    {
                        pNode->childs[i] = pNode->childs[i-1];
                        pNode->keys[i] = pNode->keys[i-1];
                    }
                    pNode->childs[0] = brother->childs[brother->count];
                    pNode->keys[0] = parent->keys[index];
                    pNode->count ++;

                    parent->keys[index]= brother->keys[brother->count-1];

                    if(brother->childs[brother->count])
                    {
                        brother->childs[brother->count]->parent = pNode;
                    }
                    brother->removeAt(brother->count-1);

                    return true;

                }
                else
                {

                    brother->keys[brother->count] = parent->keys[index];
                    parent->removeAt(index);
                    brother->count ++;

                    for(int i = 0;i < pNode->count;i ++)
                    {
                        brother->childs[brother->count+i] = pNode->childs[i];
                        brother->keys[brother->count+i] = pNode->keys[i];
                        brother->childs[brother->count+i]-> parent= brother;
                    }
                    brother->childs[brother->count+pNode->count] = pNode->childs[pNode->count];
                    brother->childs[brother->count+pNode->count]->parent = brother;

                    brother->count += pNode->count;


                    delete pNode;
                    nodeCount --;

                    return adjustAfterDelete(parent);
                }

            }
            else
            {
                if(parent->childs[0] == pNode)
                    brother = parent->childs[1];
                else
                    brother = parent->childs[index+2];
                if(brother->count > minmumKey - 1)
                {

                    pNode->childs[pNode->count+1] = brother->childs[0];
                    pNode->keys[pNode->count] = brother->keys[0];
                    pNode->childs[pNode->count+1]->parent = pNode;
                    pNode->count ++;

                    if(pNode == parent->childs[0])
                        parent->keys[0] = brother->keys[0];
                    else
                        parent->keys[index+1] = brother->keys[0];

                    brother->childs[0] = brother->childs[1];
                    brother->removeAt(0);

                    return true;
                }
                else
                {

                    pNode->keys[pNode->count] = parent->keys[index];

                    if(pNode == parent->childs[0])
                        parent->removeAt(0);
                    else
                        parent->removeAt(index+1);

                    pNode->count ++;

                    for(int i = 0;i < brother->count;i++)
                    {
                        pNode->childs[pNode->count+i] = brother->childs[i];
                        pNode->keys[pNode->count+i] = brother->keys[i];
                        pNode->childs[pNode->count+i]->parent = pNode;
                    }
                    pNode->childs[pNode->count+brother->count] = brother->childs[brother->count];
                    pNode->childs[pNode->count+brother->count]->parent = pNode;

                    pNode->count += brother->count;


                    delete brother;
                    nodeCount --;

                    return adjustAfterDelete(parent);

                }

            }

        }

    }
    return false;
}

//删除指定根节点的树

template <class KeyType>
void BPlusTree<KeyType>::dropTree(Node node)
{
    if(!node) return;
    if(!node->isLeaf)
    {
        for(size_t i=0;i <= node->count;i++)
        {
            dropTree(node->childs[i] );
            node->childs[i] = NULL;
        }
    }
    delete node;
    nodeCount --;
    return;
}

//从磁盘读取整个树

template <class KeyType>
void BPlusTree<KeyType>::readFromDiskAll()
{
    file = bm.getFile(fileName.c_str());
    blockNode* btmp = bm.getBlockHead(file);
    while (true)
    {
        if (btmp == NULL)
        {
            return;
        }

        readFromDisk(btmp);
        if(btmp->ifbottom) break;
        btmp = bm.getNextBlock(file, btmp);
    }

}

//从磁盘读取一个节点

template <class KeyType>
void BPlusTree<KeyType>::readFromDisk(blockNode* btmp)
{
    int valueSize = sizeof(offsetNumber);
    char* indexBegin = bm.get_content(*btmp);
    char* valueBegin = indexBegin + keySize;
    KeyType key;
    offsetNumber value;

    while(valueBegin - bm.get_content(*btmp) < bm.get_usingSize(*btmp))
        // there are available position in the block
    {
        key = *(KeyType*)indexBegin;
        value = *(offsetNumber*)valueBegin;
        insertKey(key, value);
        valueBegin += keySize + valueSize;
        indexBegin += keySize + valueSize;
    }

}

//把整个树存放到磁盘

template <class KeyType>
void BPlusTree<KeyType>::writtenbackToDiskAll()
{
    blockNode* btmp = bm.getBlockHead(file);
    Node ntmp = leafHead;
    int valueSize = sizeof(offsetNumber);
    while(ntmp != NULL)
    {
        bm.set_usingSize(*btmp, 0);
        bm.set_dirty(*btmp);
        for(int i = 0;i < ntmp->count;i ++)
        {
            char* key = (char*)&(ntmp->keys[i]);
            char* value = (char*)&(ntmp->vals[i]);
            memcpy(bm.get_content(*btmp)+bm.get_usingSize(*btmp),key,keySize);
            bm.set_usingSize(*btmp, bm.get_usingSize(*btmp) + keySize);
            memcpy(bm.get_content(*btmp)+bm.get_usingSize(*btmp),value,valueSize);
            bm.set_usingSize(*btmp, bm.get_usingSize(*btmp) + valueSize);
        }

        btmp = bm.getNextBlock(file, btmp);
        ntmp = ntmp->nextLeafNode;
    }
    while(1)// 删除空节点
    {
        if(btmp->ifbottom)
            break;
        bm.set_usingSize(*btmp, 0);
        bm.set_dirty(*btmp);
        btmp = bm.getNextBlock(file, btmp);
    }

}

#ifdef _DEBUG

template <class KeyType>
void BPlusTree<KeyType>::debug_print()
{
    cout << "############DEBUG FOR THE TREE############" << endl;
    cout << "name:" << fileName << " root:" << (void*)root << " leafHead:" << (void * )leafHead << " keycount:" << keyCount << " level:" << level << " nodeCount:" << nodeCount << endl;

    if(root)
        debug_print_node(root);
    cout << "#############END OF DEBUG FOR TREE########" << endl;

}


template <class KeyType>
void BPlusTree<KeyType>::debug_print_node(Node pNode)
{
    pNode->debug_print();
    if(!pNode->isLeaf)
        for(int i = 0 ;i < pNode->count+1;i ++)
            debug_print_node(pNode->childs[i]);

}
#endif


#endif /* defined(__Minisql__BPlusTree__) */
