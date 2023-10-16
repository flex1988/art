#include <emmintrin.h>
#include <vector>
#include <queue>
#include "adaptive_radix_tree.h"
#include "assert.h"
#include "stdio.h"

namespace art
{

int AdaptiveRadixTree::checkPrefix(Node* node, const unsigned char* key, int depth)
{
    int i;
    for (i = 0; i < 6 && i < node->prefix_length; i++)
    {
        if (key[depth + i] != node->prefix[i])
            return i;
    }
    return i;
}

uint32_t AdaptiveRadixTree::maxCapacitySize(NodeType type)
{
    switch (type)
    {
        case NODE4:
            return 4;
        case NODE16:
            return 16;
        case NODE48:
            return 48;
        case NODE256:
            return 256;
    }
    assert(0);
}

Node4* AdaptiveRadixTree::makeNode4()
{
    _used_memory += sizeof(Node4);
    return new Node4;
}

Node16* AdaptiveRadixTree::makeNode16()
{
    _used_memory += sizeof(Node16);
    return new Node16;
}

Node48* AdaptiveRadixTree::makeNode48()
{
    _used_memory += sizeof(Node48);
    return new Node48;
}

Node256* AdaptiveRadixTree::makeNode256()
{
    _used_memory += sizeof(Node256);
    return new Node256;
}

Node* AdaptiveRadixTree::makeNode(NodeType type)
{
    switch (type)
    {
        case NODE4:
            return reinterpret_cast<Node*>(makeNode4());
        case NODE16:
            return reinterpret_cast<Node*>(makeNode16());
        case NODE48:
            return reinterpret_cast<Node*>(makeNode48());
        case NODE256:
            return reinterpret_cast<Node*>(makeNode256());
    }
}

void AdaptiveRadixTree::freeNode(Node* node)
{
    switch (node->type)
    {
        case NODE4:
            _used_memory -= sizeof(Node4);
            break;
        case NODE16:
            _used_memory -= sizeof(Node16);
            break;
        case NODE48:
            _used_memory -= sizeof(Node48);
            break;
        case NODE256:
            _used_memory -= sizeof(Node256);
            break;
        default:
            assert(0);
    }
    delete node;
}

// 忽略重复的key，直接伸展到可以容纳的nodetype
void AdaptiveRadixTree::addLeafChild(Node* node, Node** ref, unsigned char start, uint32_t length, void* val)
{
    assert(node->is_leaf);
    uint32_t total = node->child_count + length;
    if (total <= maxCapacitySize(node->type) || node->type == NODE256)
    {
        addLeafChildSafe(node, ref, start, length, val);
    }
    else
    {
        Node* newNode = expandLeafChild(node, total);
        // 扩容路径不可能有NODE4
        assert(newNode->type != NODE4);
        addLeafChildSafe(newNode, ref, start, length, val);
        *ref = newNode;
    }
}

// 4 -> 16, 4 -> 48, 4 -> 256
// 16 -> 48, 16 -> 256
// 48 -> 256
Node* AdaptiveRadixTree::expandLeafChild(Node* node, uint32_t expected_size)
{
    if (expected_size > 48)
    {
        Node256* newNode = makeNode256();
        memcpy(&newNode->header, node, sizeof(Node));
        newNode->header.type = NODE256;
        newNode->header.child_count = 0;
        switch (node->type)
        {
            case NODE4:
            {
                Node4* node4 = reinterpret_cast<Node4*>(node);
                for (int i = 0; i < node->child_count; i++)
                {
                    addChild256(newNode, NULL, node4->child_keys[i], node4->child_ptrs[i]);
                }
                break;
            }
            case NODE16:
            {
                Node16* node16 = reinterpret_cast<Node16*>(node);
                for (int i = 0; i < node->child_count; i++)
                {
                    addChild256(newNode, NULL, node16->child_keys[i], node16->child_ptrs[i]);
                }
                break;
            }
            case NODE48:
            {
                Node48* node48 = reinterpret_cast<Node48*>(node);
                for (int i = 0; i < 256; i++)
                {
                    if (node48->child_ptr_indexs[i] > 0)
                    {
                        addChild256(newNode, NULL, i, node48->child_ptrs[node48->child_ptr_indexs[i] - 1]);
                    }
                }
                break;
            }
        }
        freeNode(node);
        return reinterpret_cast<Node*>(newNode);
    }
    else if (expected_size > 16)
    {
        Node48* newNode = makeNode48();
        memcpy(&newNode->header, node, sizeof(Node));
        newNode->header.type = NODE48;
        newNode->header.child_count = 0;
        switch (node->type)
        {
            case NODE4:
            {
                Node4* node4 = reinterpret_cast<Node4*>(node);
                for (int i = 0; i < node->child_count; i++)
                {
                    addChild48(newNode, NULL, node4->child_keys[i], node4->child_ptrs[i]);
                }
                break;
            }
            case NODE16:
            {
                Node16* node16 = reinterpret_cast<Node16*>(node);
                for (int i = 0; i < node->child_count; i++)
                {
                    addChild48(newNode, NULL, node16->child_keys[i], node16->child_ptrs[i]);
                }
                break;
            }
        }
        freeNode(node);
        return reinterpret_cast<Node*>(newNode);
    }
    else
    {
        Node16* newNode = makeNode16();
        assert(node->type == NODE4);
        Node4* node4 = reinterpret_cast<Node4*>(node);
        memcpy(&newNode->header, node, sizeof(Node));
        newNode->header.type = NODE16;
        newNode->header.child_count = 0;
        for (int i = 0; i < node->child_count; i++)
        {
            addChild16(newNode, NULL, node4->child_keys[i], node4->child_ptrs[i]);
        }
        freeNode(node);
        return reinterpret_cast<Node*>(newNode);
    }
}

void AdaptiveRadixTree::addLeafChildSafe(Node* node, Node** ref, unsigned char start, uint32_t length, void* val)
{
    switch (node->type)
    {
        case NODE4:
        {
            return addLeafChild4(node, ref, start, length, val);
        }
        case NODE16:
        {
            return addLeafChild16(reinterpret_cast<Node16*>(node), start, length, val);
        }
        case NODE48:
        {
            return addLeafChild48(node, ref, start, length, val);
        }
        case NODE256:
        {
            return addLeafChild256(node, ref, start, length, val);
        }
    }
}

void AdaptiveRadixTree::addLeafChild256(Node* node, Node** ref, unsigned char start, uint32_t length, void* val)
{
    Node256* node256 = reinterpret_cast<Node256*>(node);
    void* cursor = val;
    for (int i = 0; i < length; i++)
    {
        if (node256->child_ptrs[start + i] == NULL)
        {
            node->child_count++;
        }
        node256->child_ptrs[start + i] = reinterpret_cast<Node*>(cursor);
    }
    assert(node->child_count <= 256);
}

void AdaptiveRadixTree::addLeafChild48(Node* node, Node** ref, unsigned char start, uint32_t length, void* val)
{
    Node48* node48 = reinterpret_cast<Node48*>(node);
    int insert_index = 0;
    void* cursor = val;
    for (int i = 0; i < length; i++)
    {
        if (node48->child_ptr_indexs[start + i] > 0)
        {
            node48->child_ptrs[node48->child_ptr_indexs[start + i] - 1] = reinterpret_cast<Node*>(cursor);
        }
        else
        {
            while (node48->child_ptrs[insert_index]) insert_index++;
            node48->child_ptr_indexs[start + i] = insert_index + 1;
            node48->child_ptrs[insert_index] = reinterpret_cast<Node*>(cursor);
            insert_index++;
            node->child_count++;
        }
    }
    assert(node->child_count <= 48);
}

void AdaptiveRadixTree::addLeafChild16(Node16* node16, unsigned char start, uint32_t length, void* val)
{
    Node* node = reinterpret_cast<Node*>(node16);

    void* cursor = val;
    int start_index = -1;
    int end_index = -1;
    int end = start + length - 1;
    // SSE指令_mm_cmplt_epi8 _mm_cmpgt_epi8是针对有符号数的函数
    // 如果使用_mm_cmplt_epi16，那么需要两条SSE指令，所以这里可以先不用SSE指令实现
    if (node->child_count > 0)
    {
        // __m128i childs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(node16->child_keys));
        // __m128i cmpend = _mm_cmplt_epi8(_mm_set1_epi8(end), childs);
        // int bitfieldend = _mm_movemask_epi8(cmpend) & mask;
        // if (bitfieldend)
        // {
        //     end_index = __builtin_ctz(bitfieldend);
        // }
        // __m128i cmpstart = _mm_cmpgt_epi8(_mm_set1_epi8(start), childs);
        // int bitfieldstart = _mm_movemask_epi8(cmpstart) & mask;
        // if (bitfieldstart)
        // {
        //     start_index = 32 - __builtin_clz(bitfieldstart) - 1;
        // }

        for (int i = 0; i < node->child_count; i++)
        {
            if (node16->child_keys[i] >= start)
            {
                break;
            }
            start_index = i;
        }

        for (int j = node->child_count - 1; j >= 0; j--)
        {
            if (node16->child_keys[j] <= end)
            {
                break;
            }
            end_index = j;
        }
    }
    
    // 如果比start小的没找到，而且比end大的没找到，说明start-end可以把当前所有的child都覆盖
    if (start_index == -1 && end_index == -1)
    {
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[i] = start + i;
            node16->child_ptrs[i] = reinterpret_cast<Node*>(cursor);
        }
        node->child_count = length;
    }
    // start和end都找到了，需要将end后面的元素整体向右挪出空间能放下start-end
    else if (start_index != -1 && end_index != -1)
    {
        int movelen = length - (end_index - start_index - 1);
        assert(node->child_count > end_index);
        memmove(&node16->child_keys[end_index + movelen], &node16->child_keys[end_index], node->child_count - end_index);
        memmove(&node16->child_ptrs[end_index + movelen], &node16->child_ptrs[end_index], (node->child_count - end_index) * sizeof(void*));
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[start_index + i + 1] = start + i;
            node16->child_ptrs[start_index + i + 1] = reinterpret_cast<Node*>(cursor);
        }
        node->child_count += movelen;

    }
    else if (start_index == -1)
    {
        int movelen = length - end_index;
        assert(node->child_count > 0);
        memmove(&node16->child_keys[end_index + movelen], &node16->child_keys[end_index], node->child_count);
        memmove(&node16->child_ptrs[end_index + movelen], &node16->child_ptrs[end_index], node->child_count * sizeof(void*));
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[i] = start + i;
            node16->child_ptrs[i] = reinterpret_cast<Node*>(cursor);
        }
        node->child_count += movelen;
    }
    // start找到了，end没找到，从start开始连续插入就可以了
    else if (end_index == -1)
    {
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[start_index + i + 1] =  start + i;
            node16->child_ptrs[start_index + i + 1] = reinterpret_cast<Node*>(cursor);
        }   
        node->child_count = start_index + 1 + length;
    }
    else
    {
        assert(0);
    }
    assert(node->child_count <= 16);
}

// 剩余容量是绝对够的
void AdaptiveRadixTree::addLeafChild4(Node* node, Node** ref, unsigned char start, uint32_t length, void* val)
{
    Node4* node4 = reinterpret_cast<Node4*>(node);
    int inserted = 0;
    int i = 0;
    for (; i < node->child_count; i++)
    {
        if (node4->child_keys[i] == start + inserted)
        {
            node4->child_ptrs[i] = reinterpret_cast<Node*>(val);
            inserted++;
        }
        else if (node4->child_keys[i] > start + inserted)
        {
            assert(node->child_count > i);
            memmove(&node4->child_keys[i + 1], &node4->child_keys[i], node->child_count - i);
            memmove(&node4->child_ptrs[i + 1], &node4->child_ptrs[i], sizeof(void*) * (node->child_count - i));
            node4->child_keys[i] = start + inserted;
            node4->child_ptrs[i] = reinterpret_cast<Node*>(val);
            inserted++;
            node->child_count++;
        }

        if (inserted == length)
        {
            break;
        }
    }
    while (inserted < length)
    {
        node4->child_keys[i] = start + inserted;
        node4->child_ptrs[i] = reinterpret_cast<Node*>(val);
        node->child_count++;
        i++;
        inserted++;
    }
    assert(node->child_count <= 4);
}

void AdaptiveRadixTree::addChild(Node* node, Node** ref, unsigned char byte, void* child)
{
    // printf("addChild %p %d %d %d\n", node, byte, node->type, node->child_count);
    switch (node->type)
    {
        case NODE4:
        {
            return addChild4(reinterpret_cast<Node4*>(node), ref, byte, child);
        }
        case NODE16:
        {
            return addChild16(reinterpret_cast<Node16*>(node), ref, byte, child);
        }
        case NODE48:
        {
            return addChild48(reinterpret_cast<Node48*>(node), ref, byte, child);
        }
        case NODE256:
        {
            return addChild256(reinterpret_cast<Node256*>(node), ref, byte, child);
        }
        assert(0);
    }
}

void AdaptiveRadixTree::addChild4(Node4* node4, Node** ref, unsigned char byte, void* child)
{
    Node* node = reinterpret_cast<Node*>(node4);
    bool found = false;
    int i;
    for (i = 0; i < node->child_count; i++)
    {
        if (byte == node4->child_keys[i])
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        assert(node4->child_ptrs[i]);
        node4->child_ptrs[i] = reinterpret_cast<Node*>(child);
        return;
    }

    if (node->child_count < 4)
    {
        if (node->child_count == 0)
        {
            node4->child_keys[0] = byte;
            node4->child_ptrs[0] = reinterpret_cast<Node*>(child);
            node->child_count++;
            return;
        }

        int slot;
        for (slot = 0; slot < node->child_count; slot++)
        {
            if (byte < node4->child_keys[slot])
            {
                break;
            }
        }

        if (node->child_count > slot)
        {
            memmove(&node4->child_keys[slot+1], &node4->child_keys[slot], node->child_count - slot);
            memmove(&node4->child_ptrs[slot+1], &node4->child_ptrs[slot], (node->child_count - slot) * sizeof(void*));
        }
        node4->child_keys[slot] = byte;
        node4->child_ptrs[slot] = reinterpret_cast<Node*>(child);
        node->child_count++;
    }
    else
    {
        assert(node->child_count == 4);
        Node16* newNode = makeNode16();
        memcpy(&newNode->child_keys[0], &node4->child_keys[0], node->child_count);
        memcpy(&newNode->child_ptrs[0], &node4->child_ptrs[0], node->child_count * sizeof(void*));
        memcpy(&newNode->header, &node4->header, sizeof(Node));
        newNode->header.type = NODE16;
        assert(*ref == node);
        *ref = reinterpret_cast<Node*>(newNode);
        freeNode(node);
        addChild16(newNode, NULL, byte, child);
    }
}

void AdaptiveRadixTree::addChild16(Node16* node16, Node** ref, unsigned char byte, void* child)
{
    Node* node = reinterpret_cast<Node*>(node16);
    bool found = false;
    int i;
    for (i = 0; i < node->child_count; i++)
    {
        if (byte == node16->child_keys[i])
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        assert(node16->child_ptrs[i]);
        node16->child_ptrs[i] = reinterpret_cast<Node*>(child);
        return;
    }
    if (node->child_count < 16)
    {
        if (node->child_count == 0)
        {
            node16->child_keys[0] = byte;
            node16->child_ptrs[0] = reinterpret_cast<Node*>(child);
            node->child_count++;
            return;
        }
        int mask = (1 << node->child_count) - 1;
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(byte),
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(node16->child_keys)));
        int bitfield = _mm_movemask_epi8(cmp);
        uint32_t idx;
        if (bitfield)
        {
            idx = __builtin_ctz(bitfield);
            if (node->child_count > idx)
            {
                memmove(&node16->child_keys[idx + 1], &node16->child_keys[idx], node->child_count - idx);
                memmove(&node16->child_ptrs[idx + 1], &node16->child_ptrs[idx], (node->child_count - idx) * sizeof(void*));
            }
        }
        else
        {
            idx = node->child_count;
        }

        node16->child_keys[idx] = byte;
        node16->child_ptrs[idx] = reinterpret_cast<Node*>(child);
        node->child_count++;
    }
    else
    {
        assert(node->child_count == 16);
        Node48* newNode = makeNode48();
        memcpy(&newNode->child_ptrs[0], &node16->child_ptrs[0], node->child_count * sizeof(void*));
        for (int i = 0; i < node->child_count; i++)
        {
            newNode->child_ptr_indexs[node16->child_keys[i]] = i + 1;
        }
        memcpy(&newNode->header, &node16->header, sizeof(Node));
        newNode->header.type = NODE48;
        *ref = reinterpret_cast<Node*>(newNode);
        freeNode(node);
        addChild48(newNode, ref, byte, child);
    }
}

void AdaptiveRadixTree::addChild48(Node48* node48, Node** ref, unsigned char byte, void* child)
{
    Node* node = reinterpret_cast<Node*>(node48);
    if (node48->child_ptr_indexs[byte] > 0)
    {
        assert(node48->child_ptrs[node48->child_ptr_indexs[byte] - 1]);
        node48->child_ptrs[node48->child_ptr_indexs[byte] - 1] = reinterpret_cast<Node*>(child);
        return;
    }
         
    if (node48->header.child_count < 48)
    {
        if (node->child_count == 0)
        {
            node48->child_ptr_indexs[byte] = 1;
            node48->child_ptrs[0] = reinterpret_cast<Node*>(child);
            node->child_count++;
            return;
        }
        int pos = 0;
        while (node48->child_ptrs[pos]) pos++;
        node48->child_ptrs[pos] = reinterpret_cast<Node*>(child);
        assert(node48->child_ptr_indexs[byte] == 0);
        node48->child_ptr_indexs[byte] = pos + 1;
        node48->header.child_count++;
    }
    else
    {
        assert(node->child_count == 48);
        Node256* newNode = makeNode256();
        for (int i = 0; i < 256; i++)
        {
            if (node48->child_ptr_indexs[i])
            {
                newNode->child_ptrs[i] = node48->child_ptrs[node48->child_ptr_indexs[i] - 1];
            }
        }
        memcpy(&newNode->header, &node48->header, sizeof(Node));
        newNode->header.type = NODE256;
        *ref = reinterpret_cast<Node*>(newNode);
        freeNode(node);
        addChild256(newNode, NULL, byte, child);
    }
}

void AdaptiveRadixTree::addChild256(Node256* node256, Node** ref, unsigned char byte, void* child)
{
    (void)ref;
    assert(child);
    if (node256->child_ptrs[byte] == NULL)
    {
        node256->header.child_count++;
    }
    node256->child_ptrs[byte] = reinterpret_cast<Node*>(child);
}

Node** AdaptiveRadixTree::findChild(Node* node, unsigned char byte)
{
    switch (node->type)
    {
        case NODE4:
        {
            Node4* n = reinterpret_cast<Node4*>(node);
            for (int i = 0; i < node->child_count; i++)
            {
                if (n->child_keys[i] == byte)
                {
                    return &n->child_ptrs[i];
                }
            }
            return NULL;
        }
        case NODE16:
        {
            Node16* n = reinterpret_cast<Node16*>(node);
            __m128i results = _mm_cmpeq_epi8(_mm_set1_epi8(byte), _mm_loadu_si128((__m128i*)(&n->child_keys[0])));
            int mask = (1 << node->child_count) - 1;
            int bitfield = _mm_movemask_epi8(results) & mask;
            if (bitfield == 0)
            {
                return NULL;
            }
            return &n->child_ptrs[__builtin_ctz(bitfield)];
        }
        case NODE48:
        {
            Node48* n = reinterpret_cast<Node48*>(node);
            int index = n->child_ptr_indexs[byte];
            if (index == 0)
            {
                return NULL;
            }
            return &n->child_ptrs[index - 1];
        }
        case NODE256:
        {
            Node256* n = reinterpret_cast<Node256*>(node);
            return &n->child_ptrs[byte];
        }
        assert(0);
    }
    return NULL;
}

void printkey(uint64_t key)
{
    char* data = (char*)&key;
    printf("%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
}


Node* AdaptiveRadixTree::makeProperNode(uint32_t length)
{
    Node* newNode;
    if (length < 5)
    {
        newNode = reinterpret_cast<Node*>(makeNode4());
    }
    else if (length < 17)
    {
        newNode = reinterpret_cast<Node*>(makeNode16());
    }
    else if (length < 49)
    {
        newNode = reinterpret_cast<Node*>(makeNode48());
    }
    else
    {
        newNode = reinterpret_cast<Node*>(makeNode256());
    }
    return newNode;
}

void AdaptiveRadixTree::insert(Node* node, Node** ref, unsigned char* key, uint32_t length, void* val, int depth)
{
    if (node == NULL)
    {
        Node* newNode = makeProperNode(length);
        if (depth < 7)
        {
            memcpy(&newNode->prefix[0], &key[depth], 8 - depth - 1);
            newNode->prefix_length = 8 - depth - 1;
            assert(newNode->prefix_length <= 8);
        }
        newNode->is_leaf = true;
        addLeafChild(newNode, &newNode, key[7], length, val);
        *ref = newNode;
        return;
    }

    do
    {
        if (node->prefix_length > 0 && depth < 7)
        {
            int p = checkPrefix(node, key, depth);
            assert(node->prefix_length <= 7);
            // p不可能大于node->prefix_length
            if (p == node->prefix_length)
            {
                depth += node->prefix_length;
                break;
            }

            Node* newNode = reinterpret_cast<Node*>(makeNode4());
            *ref = newNode;
            newNode->prefix_length = p;
            assert(newNode->prefix_length <= 8);

            if (p > 0)
            {
                memcpy(&newNode->prefix[0], &node->prefix[0], p);
            }
            unsigned char oldByte = node->prefix[p];
            node->prefix_length -= (p + 1);
            if (node->prefix_length > 0)
            {
                memmove(&node->prefix[0], &node->prefix[0] + p + 1, node->prefix_length);
            }
            assert(node->prefix_length < 7);

            Node* leafNode = makeProperNode(length);
            // 去掉第一个和最后一个，前缀6减去公共前缀长度就是分裂后的长度
            leafNode->prefix_length = 8 - depth - p - 2;
            memcpy(&leafNode->prefix[0], &key[depth + p + 1], leafNode->prefix_length);
            assert(leafNode->prefix_length < 7);
            leafNode->is_leaf = true;
            addLeafChild(leafNode, &leafNode, key[7], length, val);
            addChild(newNode, NULL, key[depth + p], leafNode);
            addChild(newNode, NULL, oldByte, node);
            return;
        }
    } while (0);

    if (depth == 7)
    {
        addLeafChild(node, ref, key[depth], length, val);
        return;
    }

    Node** next = findChild(node, key[depth]);
    if (next)
    {
        // 如果当前槽位是空的，需要插入一个child
        if (*next == NULL)
        {
            node->child_count++;
        }
        insert(*next, next, key, length, val, depth+1);
    }
    else
    {
        Node* newNode = makeProperNode(length);
        assert(8 > depth - 2);
        memcpy(&newNode->prefix[0], &key[depth + 1], 8 - depth - 2);
        newNode->prefix_length = 8 - depth - 2;
        assert(newNode->prefix_length <= 8);
        newNode->is_leaf = true;
        addLeafChild(newNode, &newNode, key[7], length, val);
        addChild(node, ref, key[depth], newNode);
    }
}


void AdaptiveRadixTree::findLeafChild(Node* node, unsigned char start, uint32_t length, std::vector<void*>* vals)
{
    switch (node->type)
    {
        case NODE4:
        {
            return findLeafChild4(reinterpret_cast<Node4*>(node), start, length, vals);
        }
        case NODE16:
        {
            return findLeafChild16(reinterpret_cast<Node16*>(node), start, length, vals);
        }
        case NODE48:
        {
            return findLeafChild48(reinterpret_cast<Node48*>(node), start, length, vals);
        }
        case NODE256:
        {
            return findLeafChild256(reinterpret_cast<Node256*>(node), start, length, vals);
        }
    }
}

void AdaptiveRadixTree::findLeafChild4(Node4* node, unsigned char start, uint32_t length, std::vector<void*>* vals)
{
    unsigned char target = start;
    uint32_t last = length;
    vals->resize(length);
    for (int i = 0; i < node->header.child_count; i++)
    {
        if (node->child_keys[i] == target)
        {
            (*vals)[length - last] = node->child_ptrs[i];
            target++;
            last--;
        }
        if (last == 0)
        {
            break;
        }
    }
}

void AdaptiveRadixTree::findLeafChild16(Node16* node, unsigned char start, uint32_t length, std::vector<void*>* vals)
{
    unsigned char target = start;
    uint32_t last = length;
    vals->resize(length);
    for (int i = 0; i < node->header.child_count; i++)
    {
        if (node->child_keys[i] == target)
        {
            (*vals)[length - last] = node->child_ptrs[i];
            target++;
            last--;
        }
        if (last == 0)
        {
            break;
        }
    }
}

void AdaptiveRadixTree::findLeafChild48(Node48* node, unsigned char start, uint32_t length, std::vector<void*>* vals)
{
    unsigned char target = start;
    do
    {
        if (node->child_ptr_indexs[target] > 0)
        {
            vals->push_back(node->child_ptrs[node->child_ptr_indexs[target] - 1]);
        }
        else
        {
            vals->push_back(0);
        }
        if (target == start + length - 1)
        {
            break;
        }
        target++;
    } while (1);
}

void AdaptiveRadixTree::findLeafChild256(Node256* node, unsigned char start, uint32_t length, std::vector<void*>* vals)
{
    vals->resize(length);
    memcpy(&(*vals)[0], &node->child_ptrs[start], length * sizeof(void*));
}

void AdaptiveRadixTree::Init()
{
    _root = reinterpret_cast<Node*>(makeNode4());
    _max_node_persistent_size = std::max(sizeof(Node4Persistent), sizeof(Node16Persistent));
    _max_node_persistent_size = std::max(_max_node_persistent_size, sizeof(Node48Persistent));
    _max_node_persistent_size = std::max(_max_node_persistent_size, sizeof(Node256Persistent));
    _max_node_persistent_size = std::max(_max_node_persistent_size, sizeof(Node4LeafPersistent));
    _max_node_persistent_size = std::max(_max_node_persistent_size, sizeof(Node16LeafPersistent));
    _max_node_persistent_size = std::max(_max_node_persistent_size, sizeof(Node48LeafPersistent));
    _max_node_persistent_size = std::max(_max_node_persistent_size, sizeof(Node256LeafPersistent));
}

void* AdaptiveRadixTree::Search(uint64_t key)
{
    Node* node = _root;
    uint64_t reverse = __builtin_bswap64(key);
    unsigned char* data = reinterpret_cast<unsigned char*>(&reverse);
    int depth = 0;
    while (node && depth < 8)
    {
        if (node->prefix_length > 0)
        {
            int p = checkPrefix(node, data, depth);
            if (p != node->prefix_length)
            {
                return NULL;
            }
            depth += node->prefix_length;
        }

        Node** ref = findChild(node, data[depth]);
        node = (ref == NULL) ? NULL : *ref;

        depth++;
    }
    return node;
}

void AdaptiveRadixTree::Insert(uint64_t key, void* val)
{
    uint64_t reverse = __builtin_bswap64(key);

    insert(_root, &_root, reinterpret_cast<unsigned char*>(&reverse), 1, val, 0);
}


// 需要保证[start, start + length]在同一个叶节点
void AdaptiveRadixTree::RangeInsert(uint64_t start, uint32_t length, void* val)
{
    assert(start % 256 + length <= 256);

    uint64_t reverse = __builtin_bswap64(start);

    insert(_root, &_root, reinterpret_cast<unsigned char*>(&reverse), length, val, 0);
}

void AdaptiveRadixTree::RangeQuery(uint64_t start, uint32_t length, std::vector<void*>* vals)
{
    assert(start % 256 + length <= 256);
    Node* node = _root;
    uint64_t reverse = __builtin_bswap64(start);
    unsigned char* data = reinterpret_cast<unsigned char*>(&reverse);
    int depth = 0;
    while (node && depth < 8)
    {
        if (node->prefix_length > 0)
        {
            int p = checkPrefix(node, data, depth);
            if (p != node->prefix_length)
            {
                return;
            }
            depth += node->prefix_length;
        }

        if (depth == 7)
        {
            findLeafChild(node, data[7], length, vals);
            assert(vals->size() == length);
            return;
        }

        Node** ref = findChild(node, data[depth]);
        node = (ref == NULL) ? NULL : *ref;

        depth++;
    }

    // 没有读到叶节点
    if (depth != 7)
    {
        assert(vals->empty());
        vals->resize(length);
    }
}

void AdaptiveRadixTree::destroyNode(Node* node, int depth)
{
    assert(node);
    if (node->child_count == 0 || depth + node->prefix_length == 7)
    {
        freeNode(node);
        return;
    }
    switch (node->type)
    {
        case NODE4:
        {
            Node4* node4 = reinterpret_cast<Node4*>(node);
            for (int i = 0; i < node->child_count; i++)
            {
                destroyNode(node4->child_ptrs[i], depth + node->prefix_length + 1);
            }
            break;
        }
        case NODE16:
        {
            Node16* node16 = reinterpret_cast<Node16*>(node);
            for (int i = 0; i < node->child_count; i++)
            {
                destroyNode(node16->child_ptrs[i], depth + node->prefix_length + 1);
            }
            break;
        }
        case NODE48:
        {
            Node48* node48 = reinterpret_cast<Node48*>(node);
            for (int i = 0; i < 48; i++)
            {
                if (node48->child_ptrs[i])
                {
                    destroyNode(node48->child_ptrs[i], depth + node->prefix_length + 1);
                }
            }
            break;
        }
        case NODE256:
        {
            Node256* node256 = reinterpret_cast<Node256*>(node);
            for (int i = 0; i < 256; i++)
            {
                if (node256->child_ptrs[i])
                {
                    destroyNode(node256->child_ptrs[i], depth + node->prefix_length + 1);
                }
            }
            break;
        }
    }
    freeNode(node);
}

void AdaptiveRadixTree::Destroy()
{
    if (!_root)
    {
        return;
    }

    destroyNode(_root, 0);
}

// 暂时不考虑buffer不够
bool AdaptiveRadixTree::serializationNode(const Node* node, char* buf, int& nodeSize)
{
    if (node->is_leaf)
    {
        switch (node->type)
        {
            case NODE4:
            {
                Node4LeafPersistent* n = reinterpret_cast<Node4LeafPersistent*>(buf);
                const Node4* node4 = reinterpret_cast<const Node4*>(node);
                memcpy(n, node, sizeof(Node));
                memcpy(&n->child_keys[0], &node4->child_keys[0], 4);
                memcpy(&n->child_ptrs[0], &node4->child_ptrs[0], 4 * sizeof(void*));
                nodeSize = sizeof(Node4LeafPersistent);
                return true;
            }
            case NODE16:
            {
                Node16LeafPersistent* n = reinterpret_cast<Node16LeafPersistent*>(buf);
                const Node16* node16 = reinterpret_cast<const Node16*>(node);
                memcpy(n, node, sizeof(Node));
                memcpy(&n->child_keys[0], &node16->child_keys[0], 16);
                memcpy(&n->child_ptrs[0], &node16->child_ptrs[0], 16 * sizeof(void*));
                nodeSize = sizeof(Node16LeafPersistent);
                return true;
            }
            case NODE48:
            {
                Node48LeafPersistent* n = reinterpret_cast<Node48LeafPersistent*>(buf);
                const Node48* node48 = reinterpret_cast<const Node48*>(node);
                memcpy(n, node48, sizeof(Node));
                memcpy(&n->child_ptr_indexs[0], &node48->child_ptr_indexs[0], 256);
                memcpy(&n->child_ptrs[0], &node48->child_ptrs[0], 48 * sizeof(void*));
                nodeSize = sizeof(Node48LeafPersistent);
                return true;
            }
            case NODE256:
            {
                Node256LeafPersistent* n = reinterpret_cast<Node256LeafPersistent*>(buf);
                const Node256* node256 = reinterpret_cast<const Node256*>(node);
                memcpy(n, node256, sizeof(Node));
                memcpy(&n->child_ptrs[0], &node256->child_ptrs[0], 256 * sizeof(void*));
                nodeSize = sizeof(Node256LeafPersistent);
                return true;
            }
        }
    }
    else 
    {
        switch (node->type)
        {
            case NODE4:
            {
                Node4Persistent* n = reinterpret_cast<Node4Persistent*>(buf);
                const Node4* node4 = reinterpret_cast<const Node4*>(node);
                memcpy(n, node, sizeof(Node));
                memcpy(&n->child_keys[0], &node4->child_keys[0], 4);
                nodeSize = sizeof(Node4Persistent);
                return true;
            }
            case NODE16:
            {
                Node16Persistent* n = reinterpret_cast<Node16Persistent*>(buf);
                const Node16* node16 = reinterpret_cast<const Node16*>(node);
                memcpy(n, node, sizeof(Node));
                memcpy(&n->child_keys[0], &node16->child_keys[0], 16);
                nodeSize = sizeof(Node16Persistent);
                return true;
            }
            case NODE48:
            {
                Node48Persistent* n = reinterpret_cast<Node48Persistent*>(buf);
                const Node48* node48 = reinterpret_cast<const Node48*>(node);
                memcpy(n, node48, sizeof(Node));
                memcpy(&n->child_ptr_indexs[0], &node48->child_ptr_indexs[0], 256);
                nodeSize = sizeof(Node48Persistent);
                return true;
            }
            case NODE256:
            {
                Node256Persistent* n = reinterpret_cast<Node256Persistent*>(buf);
                const Node256* node256 = reinterpret_cast<const Node256*>(node);
                memcpy(n, node256, sizeof(Node));
                for (int i = 0; i < 256; i++) {
                    n->child_bitmap[i] = node256->child_ptrs[i] == NULL ? 0 : 1;
                }
                nodeSize = sizeof(Node256Persistent);
                return true;
            }
        }
    }
    return false;
}

bool AdaptiveRadixTree::deserializationNode(Node** node, char** buf)
{
    Node* header = reinterpret_cast<Node*>(*buf);
    if (header->is_leaf)
    {
        switch (header->type)
        {
            case NODE4:
            {
                Node4* n4 = makeNode4();
                Node4LeafPersistent* n = reinterpret_cast<Node4LeafPersistent*>(*buf);
                memcpy(n4, header, sizeof(Node));
                memcpy(&n4->child_keys[0], &n->child_keys[0], 4);
                memcpy(&n4->child_ptrs[0], &n->child_ptrs[0], 4 * sizeof(void*));
                *node = reinterpret_cast<Node*>(n4);
                *buf += sizeof(Node4LeafPersistent);
                return true;
            }
            case NODE16:
            {
                Node16* n16 = makeNode16();
                Node16LeafPersistent* n = reinterpret_cast<Node16LeafPersistent*>(*buf);
                memcpy(n16, header, sizeof(Node));
                memcpy(&n16->child_keys[0], &n->child_keys[0], 16);
                memcpy(&n16->child_ptrs[0], &n->child_ptrs[0], 16 * sizeof(void*));
                *node = reinterpret_cast<Node*>(n16);
                *buf += sizeof(Node16LeafPersistent);
                return true;
            }
            case NODE48:
            {
                Node48* n48 = makeNode48();
                Node48LeafPersistent* n = reinterpret_cast<Node48LeafPersistent*>(*buf);
                memcpy(n48, header, sizeof(Node));
                memcpy(&n48->child_ptr_indexs[0], &n->child_ptr_indexs[0], 256);
                memcpy(&n48->child_ptrs[0], &n->child_ptrs[0], 48 * sizeof(void*));
                *node = reinterpret_cast<Node*>(n48);
                *buf += sizeof(Node48LeafPersistent);
                return true;
            }
            case NODE256:
            {
                Node256* n256 = makeNode256();
                Node256LeafPersistent* n = reinterpret_cast<Node256LeafPersistent*>(*buf);
                memcpy(n256, header, sizeof(Node));
                memcpy(&n256->child_ptrs[0], &n->child_ptrs[0], 256 * sizeof(void*));
                *node = reinterpret_cast<Node*>(n256);
                *buf += sizeof(Node256LeafPersistent);
                return true;
            }
        }
    }
    else
    {
        switch (header->type)
        {
            case NODE4:
            {
                Node4* n4 = makeNode4();
                Node4Persistent* np = reinterpret_cast<Node4Persistent*>(*buf);
                memcpy(n4, header, sizeof(Node));
                memcpy(&n4->child_keys[0], &np->child_keys[0], 4);
                *node = reinterpret_cast<Node*>(n4);
                *buf += sizeof(Node4Persistent);
                return true;
            }
            case NODE16:
            {
                Node16* n16 = makeNode16();
                Node16Persistent* np = reinterpret_cast<Node16Persistent*>(*buf);
                memcpy(n16, header, sizeof(Node));
                memcpy(&n16->child_keys[0], &np->child_keys[0], 16);
                *node = reinterpret_cast<Node*>(n16);
                *buf += sizeof(Node16Persistent);
                return true;
            }
            case NODE48:
            {
                Node48* n48 = makeNode48();
                Node48Persistent* n = reinterpret_cast<Node48Persistent*>(*buf);
                memcpy(n48, header, sizeof(Node));
                memcpy(&n48->child_ptr_indexs[0], &n->child_ptr_indexs[0], 256);
                *node = reinterpret_cast<Node*>(n48);
                *buf += sizeof(Node48Persistent);
                return true;
            }
            case NODE256:
            {
                Node256* n256 = makeNode256();
                Node256Persistent* n = reinterpret_cast<Node256Persistent*>(*buf);
                memcpy(n256, header, sizeof(Node));
                n256->child_bitmap = new Bitmap;
                memcpy(&n256->child_bitmap->bitmap[0], &n->child_bitmap[0], 256);
                *node = reinterpret_cast<Node*>(n256);
                *buf += sizeof(Node256Persistent);
                return true;
            }
        }
    }
    return false;
}

int AdaptiveRadixTree::Deserialization(const void* buf, const int bufSize)
{
    assert(_root == NULL);
    assert(bufSize > sizeof(Node));
    char* pos = (char*)buf;

    Node* n = NULL;
    assert(deserializationNode(&n, &pos));

    _root = n;

    std::queue<Node*> q;
    q.push(n);
    
    while (!q.empty())
    {
        int levelCount = q.size();
        for (int i = 0; i < levelCount; i++)
        {
            Node* parent = q.front();
            q.pop();
            if (parent->is_leaf)
            {
                continue;
            }
            int node48Index = 0;
            int node256Index = 0;

            for (int j = 0; j < parent->child_count; j++)
            {
                Node* child;
                assert(deserializationNode(&child, &pos));
                
                q.push(child);

                switch (parent->type)
                {
                    case NODE4:
                    {
                        Node4* n4 = reinterpret_cast<Node4*>(parent);
                        n4->child_ptrs[j] = child;
                        break;
                    }
                    case NODE16:
                    {
                        Node16* n16 = reinterpret_cast<Node16*>(parent);
                        n16->child_ptrs[j] = child;
                        break;
                    }
                    case NODE48:
                    {
                        Node48* n48 = reinterpret_cast<Node48*>(parent);
                        while (n48->child_ptr_indexs[node48Index] == 0) {
                            node48Index++;
                        }
                        n48->child_ptrs[n48->child_ptr_indexs[node48Index] - 1] = child;
                        node48Index++;
                        break;
                    }
                    case NODE256:
                    {
                        Node256* n256 = reinterpret_cast<Node256*>(parent);
                        while (n256->child_bitmap->bitmap[node256Index] == 0) {
                            node256Index++;
                        }
                        n256->child_ptrs[node256Index] = child;
                        node256Index++;
                        break;
                    }
                }
            }
            if (parent->type == NODE256) {
                Node256* n256 = reinterpret_cast<Node256*>(parent);
                delete n256->child_bitmap;
                n256->child_bitmap = NULL;
            }
        }
    }

    return 0;
}

// 由于不确定需要多长的buffer，所以不应该由外部申请，传进来
// 
void AdaptiveRadixTree::Serialization(void** buf, int& size)
{
    char* pos = NULL;
    int bufSize = 1 << 20;
    posix_memalign((void**)&pos, 4096, bufSize);
    *buf = pos;

    std::queue<Node*> q;
    q.push(_root);

    while (!q.empty())
    {
        int count = q.size();
        for (uint k = 0; k < count; k++)
        {
            Node* n = q.front();
            q.pop();
            switch (n->type)
            {
                case NODE4:
                {
                    Node4* n4 = reinterpret_cast<Node4*>(n);
                    if (!n->is_leaf)
                    {
                        for (uint i = 0; i < n->child_count; i++)
                        {
                            q.push(n4->child_ptrs[i]);
                        }
                    }
                    break;
                }
                case NODE16:
                {
                    Node16* n16 = reinterpret_cast<Node16*>(n);
                    if (!n->is_leaf)
                    {
                        for (uint i = 0; i < n->child_count; i++)
                        {
                            q.push(n16->child_ptrs[i]);
                        }
                    }
                    break;
                }
                case NODE48:
                {
                    Node48* n48 = reinterpret_cast<Node48*>(n);
                    if (!n->is_leaf)
                    {
                        for (uint i = 0; i < 256; i++)
                        {
                            if (n48->child_ptr_indexs[i] > 0)
                            {
                                assert(n48->child_ptrs[n48->child_ptr_indexs[i] - 1] != NULL);
                                q.push(n48->child_ptrs[n48->child_ptr_indexs[i] - 1]);
                            }
                        }
                    }
                    break;
                }
                case NODE256:
                {
                    Node256* n256 = reinterpret_cast<Node256*>(n);
                    int childCount = 0;
                    if (!n->is_leaf)
                    {
                        for (uint i = 0; i < 256; i++)
                        {
                            if (n256->child_ptrs[i])
                            {
                                q.push(n256->child_ptrs[i]);
                                childCount++;
                            }
                        }                   
                        assert(childCount == n->child_count);
                    }
                    break;
                }
                default:
                    assert(0);
            }

            if (bufSize - (pos - (char*)*buf) < _max_node_persistent_size)
            {
                bufSize *= 2;
                char* newbuf = (char*)realloc(*buf, bufSize);
                if (newbuf != *buf)
                {
                    pos = newbuf + (pos - (char*)*buf);
                    *buf = newbuf;
                }
            }

            int nodeSize = 0;
            assert(serializationNode(n, pos, nodeSize));
            pos += nodeSize;
            assert(pos - (char*)*buf < bufSize);
        }
    }

    size = (char*)pos - (char*)*buf;
}

void AdaptiveRadixTree::DumpNode(Node* node)
{
    printf("{ ");
    printf("Node: %p\t", node);
    printf("NodeType: %d\t", node->type);
    printf("ChildCount: %d\t", node->child_count);
    printf("Leaf: %d\t", node->is_leaf);
    printf("Childs: ");
    if (node->is_leaf) {
        printf(" }\n");
        return;
    }
    switch (node->type)
    {
        case NODE4:
        {
            Node4* n4 = reinterpret_cast<Node4*>(node);
            for (int i = 0; i < node->child_count; i++)
            {
                printf("[%d-%d-%p] ", i, n4->child_keys[i], n4->child_ptrs[i]);
            }
            break;
        }
        case NODE16:
        {
            Node16* n16 = reinterpret_cast<Node16*>(node);
            for (int i = 0; i < node->child_count; i++)
            {
                printf("[%d-%d-%p] ", i, n16->child_keys[i], n16->child_ptrs[i]);
            }
            break;
        }
        case NODE48:
        {
            Node48* n48 = reinterpret_cast<Node48*>(node);
            int index = 0;
            for (int i = 0; i < 256; i++)
            {
                if (n48->child_ptr_indexs[i] > 0)
                {
                    printf("[%d-%d-%p] ", index++, n48->child_ptr_indexs[i] - 1, n48->child_ptrs[n48->child_ptr_indexs[i] - 1]);
                }
            }
            break;
        }
        case NODE256:
        {
            Node256* n256 = reinterpret_cast<Node256*>(node);
            int index = 0;
            for (int i = 0; i < 256; i++)
            {
                if (n256->child_ptrs[i])
                {
                    printf("[%d-%d-%p] ", index++, i, n256->child_ptrs[i]);
                }
            }
            break;
        }
    }
    printf(" }\n");
}

void AdaptiveRadixTree::DumpTree()
{
    std::queue<Node*> q;
    q.push(_root);

    int level = 0;

    while (!q.empty())
    {
        int levelCount = q.size();
        printf("+--------------------------------+\n");
        printf("|  Level %4d   LevelCount %4d  |\n", level, levelCount);
        printf("+--------------------------------+\n");
        level++;
        for (int i = 0; i < levelCount; i++)
        {
            Node* n = q.front();
            q.pop();
            DumpNode(n);
            if (n->is_leaf)
            {
                continue;
            }
            switch (n->type)
            {
                case NODE4:
                {
                    for (int j = 0; j < n->child_count; j++)
                    {
                        Node4* n4 = reinterpret_cast<Node4*>(n);
                        q.push(n4->child_ptrs[j]);
                    }
                    break;
                }
                case NODE16:
                {
                    for (int j = 0; j < n->child_count; j++)
                    {
                        Node16* n16 = reinterpret_cast<Node16*>(n);
                        q.push(n16->child_ptrs[j]);
                    }
                    break;
                }
                case NODE48:
                {
                    Node48* n48 = reinterpret_cast<Node48*>(n);
                    for (int j = 0; j < 256; j++)
                    {
                        if (n48->child_ptr_indexs[j] > 0)
                        {
                            assert(n48->child_ptrs[n48->child_ptr_indexs[j] - 1] != NULL);
                            q.push(n48->child_ptrs[n48->child_ptr_indexs[j] - 1]);
                        }
                    }
                    break;
                }
                case NODE256:
                {
                    Node256* n256 = reinterpret_cast<Node256*>(n);
                    for (int j = 0; j < 256; j++)
                    {
                        if (n256->child_ptrs[j])
                        {
                            q.push(n256->child_ptrs[j]);
                        }
                    }                   
                    break;
                }
            }
        }
    }
    printf("\n\n");
}

}
