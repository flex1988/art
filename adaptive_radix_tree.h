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

struct Bitmap
{
    unsigned char bitmap[256];
};

struct Node
{
    uint16_t        child_count;
    uint8_t         prefix_length;
    NodeType        type : 2;
    bool            is_leaf : 1;
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
    Bitmap*         child_bitmap;
    Node256()
    {
        memset(this, 0, sizeof(*this));
        header.type = NODE256;
    }
};

struct Node4Persistent
{
    Node            header;
    unsigned char   child_keys[4];
};

struct Node16Persistent
{
    Node            header;
    unsigned char   child_keys[16];
};

struct Node48Persistent
{
    Node            header;
    unsigned char   child_ptr_indexs[256];
};

struct Node256Persistent
{
    Node            header;
    unsigned char   child_bitmap[256]; // 用来存储child和槽位的关系
};

struct Node4LeafPersistent
{
    Node            header;
    unsigned char   child_keys[4];
    Node*           child_ptrs[4];
};

struct Node16LeafPersistent
{
    Node            header;
    unsigned char   child_keys[16];
    Node*           child_ptrs[16];
};

struct Node48LeafPersistent
{
    Node            header;
    unsigned char   child_ptr_indexs[256];
    Node*           child_ptrs[48];
};

struct Node256LeafPersistent
{
    Node            header;
    Node*           child_ptrs[256];
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

    void Init();

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

    void Serialization(void** buf, int& size);

    int Deserialization(const void* buf, const int bufSize);

    void DumpNode(Node* node);

    void DumpTree();

private:
    void insert(Node* node, Node** ref, unsigned char* key, uint32_t length, void* val, int depth);

    Node4* makeNode4();
    Node16* makeNode16();
    Node48* makeNode48();
    Node256* makeNode256();
    Node* makeProperNode(uint32_t length);

    bool serializationNode(const Node* node, char* buf, int& nodeSize);

    bool deserializationNode(Node** node, char** buf);

    Node* makeNode(NodeType type);

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

    void destroyNode(Node* node, int depth);

private:

    Node*       _root;
    uint64_t    _used_memory;
    uint64_t    _total_keys;
    uint64_t    _max_node_persistent_size;
};

}
