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
typedef int offsetNumber; // 节点的值

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
    TreeNode* split(KeyType &key);
    size_t AddKey(KeyType &key); //add the key in the branch and return the position
    size_t AddKey(KeyType &key,offsetNumber val); // add a key-value in the leaf node and return the position
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
    Node LeafHeader; // 叶子节点的头
    size_t KeyCount;
    size_t level;
    size_t NodeCount;
    fileNode* file;
    int keySize;
    int degree;

public:
    BPlusTree(string m_name,int keySize,int degree);
    ~BPlusTree();

    offsetNumber search(KeyType& key);
    bool InsertKey(KeyType &key,offsetNumber val);
    bool deleteKey(KeyType &key);

    void dropTree(Node node);

    void LoadFromDiskAll();
    void WriteAlltoDisk();
    void LoadFromDisk(blockNode* btmp);

private:
    void init_tree();
    bool AdjustAfterInsert(Node pNode);
    bool AdjustAfterDelete(Node pNode);
    void FindKey(Node pNode,KeyType key,searchNodeParse &snp);

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
    if(count == 0 )
    //空节点，没有key
    {
        index = 0;
        return false;
    }
    else
    {
        // 检查key是否在范围之外
        if(keys[count-1] < key)
        {
            index = count;
            return false;
        }
        else if(keys[0] > key)
        {
            index = 0;
            return false;
        }
        else if(count <= 20)
        //如果规模较小，就顺序查找
        {
            for(size_t i = 0;i < count;i ++)
            {
                if(keys[i] == key)
                {
                    index = i;
                    return true;
                }
                else if(keys[i] > key)
                {
                    index = i;
                    return false;
                }
            }
        }
        //如果规模较大，就二分查找
        else if(count > 20)
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
            }


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
        }
    }
    return false;
}

//把当前节点分成两个
//KeyType & the key reference returns the key that will go to the upper level
template <class KeyType>
TreeNode<KeyType>* TreeNode<KeyType>::split(KeyType &key)
{
    size_t MinDegree = (degree - 1) / 2;
    TreeNode* newNode = new TreeNode(degree,this->isLeaf);


    if(isLeaf) // 这是一个叶子
    {
        key = keys[MinDegree + 1];

        //原叶子只剩MinDegree个key，其他的都给新节点
        for(size_t i = MinDegree + 1;i < degree;i ++)
        {
            newNode->keys[i-MinDegree-1] = keys[i];
            keys[i] = KeyType();
            newNode->vals[i-MinDegree-1] = vals[i];
            vals[i] = offsetNumber();
        }

        //新叶子按照顺序连在原叶子后面
        newNode->nextLeafNode = this->nextLeafNode;
        this->nextLeafNode = newNode;

        newNode->parent = this->parent;
        newNode->count = MinDegree;

        //更新degree
        this->count = MinDegree + 1;
    }
    else
    {
        key = keys[MinDegree];

        //给新节点分配孩子，原节点只剩1～MinDegree个孩子
        for(size_t i = MinDegree + 1;i < degree+1; i++)
        {
            newNode->childs[i-MinDegree-1] = this->childs[i];
            newNode->childs[i-MinDegree-1]->parent = newNode;
            this->childs[i] = NULL;
        }

        //给新节点分配key
        for(size_t i = MinDegree + 1;i < degree; i++)
        {
            newNode->keys[i-MinDegree-1] = this->keys[i];
            this->keys[i] = KeyType();
        }
        this->keys[MinDegree] = KeyType();
        newNode->parent = this->parent;
        newNode->count = MinDegree;
        this->count = MinDegree;
    }
    return newNode;
}


//向节点中添加key，返回添加的值的位置

template <class KeyType>
size_t TreeNode<KeyType>::AddKey(KeyType &key)
{

    if(count == 0)
    //如果还没有key
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
            //如果已经存在这个key
        {
            cout << "Error:In add(Keytype &key),key has already in the tree!" << endl;
            exit(3);
        }
        else
        {
            //先将所有index后的key后移，再将新key插入到index位置
            for(size_t i = count;i > index;i --)
                keys[i] = keys[i-1];
            keys[index] = key;

            //同理，调整孩子
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
size_t TreeNode<KeyType>::AddKey(KeyType &key,offsetNumber val)
{
    if(!isLeaf)
    {
        cout << "Error:add(KeyType &key,offsetNumber val) is a function for leaf nodes" << endl;
        return -1;
    }
    if(count == 0)
    //同上，没有孩子
    {
        keys[0] = key;
        vals[0] = val;
        count ++;
        return 0;
    }
    else
    {
        size_t index = 0; // key的插入位置
        bool exist = search(key, index);
        if(exist)
        {
            cout << "Error:In add(Keytype &key, offsetNumber val),key has already in the tree!" << endl;
            exit(3);
        }
        else
        {
            //后移，插入
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

//删除键值或孩子
//size_t the position to delete

template <class KeyType>
bool TreeNode<KeyType>::removeAt(size_t index)
{
    //如果key的位置超出范围
    if(index > count)
    {
        cout << "Error:In removeAt(size_t index), index is more than count!" << endl;
        return false;
    }
    else
    {
        if(isLeaf)
        {
            //index后的key前移
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
            //如果不是叶子，孩子和key都需要处理
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
BPlusTree<KeyType>::BPlusTree(string m_name,int keysize,int m_degree):fileName(m_name),KeyCount(0),level(0),NodeCount(0),root(NULL),LeafHeader(NULL),keySize(keysize),file(NULL),degree(m_degree)
{
    init_tree();
    LoadFromDiskAll();
}

//析构器
template <class KeyType>
BPlusTree<KeyType>:: ~BPlusTree()
{
    dropTree(root);
    KeyCount = 0;
    root = NULL;
    level = 0;
}

//初始化树，分配节点的空间

template <class KeyType>
void BPlusTree<KeyType>::init_tree()
{
    root = new TreeNode<KeyType>(degree,true);
    KeyCount = 0;
    level = 1;
    NodeCount = 1;
    LeafHeader = root;
}

//向叶子层搜索节点，以寻找包含值的节点

template <class KeyType>
void BPlusTree<KeyType>::FindKey(Node pNode,KeyType key,searchNodeParse & snp)
{
    size_t index = 0;
    if(pNode->search(key,index))
    //如果
    {
        if(pNode->isLeaf)
        //如果找到了叶子
        {
            snp.pNode = pNode;
            snp.index = index;
            snp.ifFound = true;
        }
        else
        {
            //向底层延伸一层，再一直找到最左边的孩子直至叶子
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
            FindKey(pNode->childs[index],key,snp);
        }
    }
}

//在合适的位置添加值，再调整结构

template <class KeyType>
bool BPlusTree<KeyType>::InsertKey(KeyType &key,offsetNumber val)
{
    searchNodeParse snp;
    if(!root) init_tree();
    FindKey(root,key,snp);
    if(snp.ifFound)
    {
//        cout << "Error:in insert key to index: the duplicated key!" << endl;
        cout << "ERROR: Duplicate entry '"<< key <<"' for key 'PRIMARY'";
        return false;
    }
    else
    {
        snp.pNode->AddKey(key,val);
        //如果已经满了,就在插入后进行调整
        if(snp.pNode->count == degree)
        {
            AdjustAfterInsert(snp.pNode);
        }
        KeyCount ++;
        return true;
    }
}



//添加后调整节点，递归调用
template <class KeyType>
bool BPlusTree<KeyType>::AdjustAfterInsert(Node pNode)
{
    KeyType key;
    Node newNode = pNode->split(key);//NewNode是将节点分割后的两个节点中的右家电
    NodeCount ++;

    //如果当前节点是根，就生成新的根，root是新根，pNode和newNode是孩子
    if(pNode->isRoot())
    {
        Node root = new TreeNode<KeyType>(degree,false);
        level ++;
        NodeCount ++;
        this->root = root;
        pNode->parent = root;
        newNode->parent = root;
        root->AddKey(key);
        root->childs[0] = pNode;
        root->childs[1] = newNode;
        return true;
    }
    else
        //如果不是根
    {
        Node parent = pNode->parent;
        size_t index = parent->AddKey(key);

        //parent是新父亲，newNode是新孩子
        parent->childs[index+1] = newNode;
        newNode->parent = parent;

        //分配完之后继续检查parent的状态
        if(parent->count == degree)
            return AdjustAfterInsert(parent);

        return true;
    }
}

//搜索树以寻找指定key

template <class KeyType>
offsetNumber BPlusTree<KeyType>::search(KeyType& key)
{
    if(!root) return -1;
    searchNodeParse snp;
    FindKey(root, key, snp);
    if(!snp.ifFound)
    {
        return -1; //没找到
    }
    else
        //返回key
    {
        return snp.pNode->vals[snp.index];
    }

}

//删除指定key，再调整结构

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
        FindKey(root, key, snp);
        if(!snp.ifFound)
        {
            cout << "ERROR: In deleteKey, no keys in the tree " << fileName << "!" << endl;
            return false;
        }
        else
        {
            if(snp.pNode->isRoot())
            //如果是根，就移除指定位置的key,再调整
            {
                snp.pNode->removeAt(snp.index);
                KeyCount --;
                return AdjustAfterDelete(snp.pNode);
            }
            else
                //如果不是根
            {
                if(snp.index == 0 && LeafHeader != snp.pNode)
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
                    KeyCount--;
                    return AdjustAfterDelete(snp.pNode);

                }
                else
                {
                    snp.pNode->removeAt(snp.index);
                    KeyCount--;
                    return AdjustAfterDelete(snp.pNode);
                }
            }
        }
    }
}

//删除后调整结构，递归调用

template <class KeyType>
bool BPlusTree<KeyType>::AdjustAfterDelete(Node pNode)
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
                LeafHeader = NULL;
                level --;
                NodeCount --;
            }
            else //根不是叶子
            {
                root = pNode -> childs[0];
                root -> parent = NULL;
                delete pNode;
                level --;
                NodeCount --;
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
                    NodeCount --;

                    return AdjustAfterDelete(parent);
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
                    NodeCount --;

                    return AdjustAfterDelete(parent);
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
                    NodeCount --;

                    return AdjustAfterDelete(parent);
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
                    NodeCount --;

                    return AdjustAfterDelete(parent);

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
    NodeCount --;
    return;
}

//从磁盘读取整个树

template <class KeyType>
void BPlusTree<KeyType>::LoadFromDiskAll()
{
    file = bm.getFile(fileName.c_str());
    blockNode* btmp = bm.GetBlockHeader(file);
    while (true)
    {
        if (btmp == NULL)
        {
            return;
        }

        LoadFromDisk(btmp);
        if(btmp->ifbottom) break;
        btmp = bm.getNextBlock(file, btmp);
    }

}

//从磁盘读取一个节点

template <class KeyType>
void BPlusTree<KeyType>::LoadFromDisk(blockNode* btmp)
{
    int valueSize = sizeof(offsetNumber);
    char* indexBegin = bm.GetContent(*btmp);
    char* valueBegin = indexBegin + keySize;
    KeyType key;
    offsetNumber value;

    while(valueBegin - bm.GetContent(*btmp) < bm.GetUsedSize(*btmp))
        //在Block中还有可用的位置
    {
        key = *(KeyType*)indexBegin;
        value = *(offsetNumber*)valueBegin;
        InsertKey(key, value);
        valueBegin += keySize + valueSize;
        indexBegin += keySize + valueSize;
    }

}

//把整个树存放到磁盘

template <class KeyType>
void BPlusTree<KeyType>::WriteAlltoDisk()
{
    blockNode* btmp = bm.GetBlockHeader(file);
    Node ntmp = LeafHeader;
    int valueSize = sizeof(offsetNumber);
    while(ntmp != NULL)
    {
        bm.SetUsedSize(*btmp, 0);
        bm.SetModified(*btmp);
        for(int i = 0;i < ntmp->count;i ++)
        {
            char* key = (char*)&(ntmp->keys[i]);
            char* value = (char*)&(ntmp->vals[i]);
            memcpy(bm.GetContent(*btmp)+bm.GetUsedSize(*btmp),key,keySize);
            bm.SetUsedSize(*btmp, bm.GetUsedSize(*btmp) + keySize);
            memcpy(bm.GetContent(*btmp)+bm.GetUsedSize(*btmp),value,valueSize);
            bm.SetUsedSize(*btmp, bm.GetUsedSize(*btmp) + valueSize);
        }

        btmp = bm.getNextBlock(file, btmp);
        ntmp = ntmp->nextLeafNode;
    }
    while(1)// 删除空节点
    {
        if(btmp->ifbottom)
            break;
        bm.SetUsedSize(*btmp, 0);
        bm.SetModified(*btmp);
        btmp = bm.getNextBlock(file, btmp);
    }

}

#ifdef _DEBUG

template <class KeyType>
void BPlusTree<KeyType>::debug_print()
{
    cout << "############DEBUG FOR THE TREE############" << endl;
    cout << "name:" << fileName << " root:" << (void*)root << " LeafHeader:" << (void * )LeafHeader << " KeyCount:" << KeyCount << " level:" << level << " NodeCount:" << NodeCount << endl;

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
