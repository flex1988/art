#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <functional>

namespace art
{

enum NodeType
{
    NODE4 = 0,
    NODE16 = 1,
    NODE48 = 2,
    NODE256 = 3
};

struct Node
{
    uint8_t         prefix_length;
    uint16_t        child_count;
    NodeType        type : 2;
    unsigned char   prefix[7];
};

struct Node4
{
    Node            header;
    unsigned char   child_keys[4];
    Node*           child_ptrs[4];
    Node4()
    {
        memset(this, 0, sizeof(*this));
        header.type = NODE4;
    }
};


struct Node16
{
    Node            header;
    unsigned char   child_keys[16];
    Node*           child_ptrs[16];
    Node16()
    {
        memset(this, 0, sizeof(*this));
        header.type = NODE16;
    }
};

struct Node48
{
    Node            header;
    unsigned char   child_ptr_indexs[256];
    Node*           child_ptrs[48];
    Node48()
    {
        memset(this, 0, sizeof(*this));
        header.type = NODE48;
    }
};

struct Node256
{
    Node            header;
    Node*           child_ptrs[256];
    Node256()
    {
        memset(this, 0, sizeof(*this));
        header.type = NODE256;
    }
};

class AdaptiveRadixTree
{

public:
    AdaptiveRadixTree()
    : _root(NULL),
      _used_memory(0),
      _total_keys(0)
    {
    }

    void Init(std::function<void(void*)> = NULL, std::function<void(void*)> = NULL);

    // 插入不会失败
    void Insert(uint64_t key, void* val);

    void* Search(uint64_t key);

    void RangeInsert(uint64_t start, uint32_t length, void* val);

    void RangeQuery(uint64_t start, uint32_t length, std::vector<void*>* vals);

    void Destroy();

    // TODO delete
    // void DeleteRange(uint64_t start, uint32_t length);

    uint64_t MemoryUsage()
    {
        return _used_memory;
    }

    uint64_t Size()
    {
        return _total_keys;
    }

private:
    void insert(Node* node, Node** ref, unsigned char* key, uint32_t length, void* val, int depth);

    Node4* makeNode4();
    Node16* makeNode16();
    Node48* makeNode48();
    Node256* makeNode256();
    Node* makeProperNode(uint32_t length);

    void freeNode(Node* node);

    void addChild(Node* node, Node** ref, unsigned char byte, void* child);
    void addChild4(Node4* node, Node** ref, unsigned char byte, void* child);
    void addChild16(Node16* node, Node** ref, unsigned char byte, void* child);
    void addChild48(Node48* node, Node** ref, unsigned char byte, void* child);
    void addChild256(Node256* node, Node** ref, unsigned char byte, void* child);

    Node* expandLeafChild(Node* node, uint32_t expected_size);

    // 需要考虑扩容
    void addLeafChild(Node* node, Node** ref, unsigned char start, uint32_t length, void* val);
    void addLeafChild4(Node* node, Node** ref, unsigned char start, uint32_t length, void* val);
    void addLeafChild16(Node16* node, unsigned char start, uint32_t length, void* val);
    void addLeafChild48(Node* node, Node** ref, unsigned char start, uint32_t length, void* val);
    void addLeafChild256(Node* node, Node** ref, unsigned char start, uint32_t length, void* val);
    // 不需要考虑扩容
    void addLeafChildSafe(Node* node, Node** ref, unsigned char start, uint32_t length, void* val);
    Node** findChild(Node* node, unsigned char byte);

    void findLeafChild(Node* node, unsigned char start, uint32_t length, std::vector<void*>* vals);
    void findLeafChild4(Node4* node, unsigned char start, uint32_t length, std::vector<void*>* vals);
    void findLeafChild16(Node16* node, unsigned char start, uint32_t length, std::vector<void*>* vals);
    void findLeafChild48(Node48* node, unsigned char start, uint32_t length, std::vector<void*>* vals);
    void findLeafChild256(Node256* node, unsigned char start, uint32_t length, std::vector<void*>* vals);

    int checkPrefix(Node* node, const unsigned char* key, int depth);
    uint32_t maxCapacitySize(NodeType type);

    void destroy_node(Node* node, int depth);

private:

    Node*       _root;
    uint64_t    _used_memory;
    uint64_t    _total_keys;
    // 用一种通用的方法解决，老的val所在的entry被插入了新值时，老的val释放的问题
    std::function<void(void*)> _deref_func;
    // 解决rangeinsert时，插入一个数组的val性能太差，而只给一个start时，val需要递增的问题
    std::function<void(void*)> _incr_func;
};

}