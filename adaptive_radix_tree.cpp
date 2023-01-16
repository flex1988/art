#include <emmintrin.h>
#include <vector>
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
        else
        {
            if (_deref_func)
            {
                _deref_func(node256->child_ptrs[start + i]);
            }
        }
        node256->child_ptrs[start + i] = reinterpret_cast<Node*>(cursor);
        if (_incr_func)
        {
            _incr_func(&cursor);
        }
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
            if (_deref_func)
            {
                _deref_func(node48->child_ptrs[node48->child_ptr_indexs[start + i] - 1]);
            }
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
        if (_incr_func)
        {
            _incr_func(&cursor);
        }
    }
    assert(node->child_count <= 48);
}

void AdaptiveRadixTree::addLeafChild16(Node16* node16, unsigned char start, uint32_t length, void* val)
{
    Node* node = reinterpret_cast<Node*>(node16);
    int inserted = 0;
    int i = 0;

    void* cursor = val;
    int start_index = -1;
    int end_index = -1;
    if (node->child_count > 0)
    {
        int end = start + length - 1;
        int mask = (1 << node->child_count) - 1;
        __m128i childs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(node16->child_keys));
        __m128i cmpend = _mm_cmplt_epi8(_mm_set1_epi8(end), childs);
        int bitfieldend = _mm_movemask_epi8(cmpend) & mask;
        if (bitfieldend)
        {
            end_index = __builtin_ctz(bitfieldend);
        }
        __m128i cmpstart = _mm_cmpgt_epi8(_mm_set1_epi8(start), childs);
        int bitfieldstart = _mm_movemask_epi8(cmpstart) & mask;
        if (bitfieldstart)
        {
            start_index = 32 - __builtin_clz(bitfieldstart) - 1;
        }
    }
    
    // 如果比start小的没找到，而且比end大的没找到，说明start-end可以把当前所有的child都覆盖
    if (start_index == -1 && end_index == -1)
    {
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[i] = start + i;
            if (_deref_func && node16->child_ptrs[i])
            {
                _deref_func(node16->child_ptrs[i]);
            }
            node16->child_ptrs[i] = reinterpret_cast<Node*>(cursor);
            if (_incr_func)
            {
                _incr_func(&cursor);
            }
        }
        node->child_count = length;
    }
    // start和end都找到了，需要将end后面的元素整体向右挪出空间能放下start-end
    else if (start_index != -1 && end_index != -1)
    {
        int movelen = length - (end_index - start_index - 1);
        memmove(&node16->child_keys[end_index + movelen], &node16->child_keys[end_index], node->child_count - end_index);
        memmove(&node16->child_ptrs[end_index + movelen], &node16->child_ptrs[end_index], (node->child_count - end_index) * sizeof(void*));
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[start_index + i + 1] = start + i;
            if (_deref_func && node16->child_ptrs[start_index + i + 1])
            {
                _deref_func(node16->child_ptrs[start_index + i + 1]);
            }
            node16->child_ptrs[start_index + i + 1] = reinterpret_cast<Node*>(cursor);
            if (_incr_func)
            {
                _incr_func(&cursor);
            }
        }
        node->child_count += movelen;

    }
    else if (start_index == -1)
    {
        int movelen = length - end_index;
        memmove(&node16->child_keys[end_index + movelen], &node16->child_keys[end_index], node->child_count);
        memmove(&node16->child_ptrs[end_index + movelen], &node16->child_ptrs[end_index], node->child_count * sizeof(void*));
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[i] = start + i;
            if (_deref_func && node16->child_ptrs[i])
            {
                _deref_func(node16->child_ptrs[i]);
            }
            node16->child_ptrs[i] = reinterpret_cast<Node*>(cursor);
            if (_incr_func)
            {
                _incr_func(&cursor);
            }
        }
        node->child_count += movelen;
    }
    // start找到了，end没找到，从start开始连续插入就可以了
    else if (end_index == -1)
    {
        for (int i = 0; i < length; i++)
        {
            node16->child_keys[start_index + i + 1] =  start + i;
            if (_deref_func && node16->child_ptrs[start_index + i + 1])
            {
                _deref_func(node16->child_ptrs[start_index + i + 1]);
            }
            node16->child_ptrs[start_index + i + 1] = reinterpret_cast<Node*>(cursor);
            if (_incr_func)
            {
                _incr_func(&cursor);
            }
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
            if (_deref_func && _deref_func)
            {
                _deref_func(node4->child_ptrs[i]);
            }
            node4->child_ptrs[i] = reinterpret_cast<Node*>(val);
            inserted++;
            if (_incr_func)
            {
                _incr_func(&val);
            }
        }
        else if (node4->child_keys[i] > start + inserted)
        {
            memmove(&node4->child_keys[i + 1], &node4->child_keys[i], node->child_count - i);
            memmove(&node4->child_ptrs[i + 1], &node4->child_ptrs[i], sizeof(void*) * (node->child_count - i));
            node4->child_keys[i] = start + inserted;
            node4->child_ptrs[i] = reinterpret_cast<Node*>(val);
            inserted++;
            node->child_count++;
            if (_incr_func)
            {
                _incr_func(&val);
            }
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
        if (_incr_func)
        {
            _incr_func(&val);
        }
    }
    assert(node->child_count <= 4);
}

void AdaptiveRadixTree::addChild(Node* node, Node** ref, unsigned char byte, void* child)
{
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
        node4->child_ptrs[i] = reinterpret_cast<Node*>(child);
        return;
    }

    if (node->child_count < 4)
    {
        int i;
        for (i = 0; i < node->child_count; i++)
        {
            if (byte < node4->child_keys[i])
            {
                break;
            }
        }

        memmove(&node4->child_keys[i+1], &node4->child_keys[i], node->child_count - i);
        memmove(&node4->child_ptrs[i+1], &node4->child_ptrs[i], (node->child_count - i) * sizeof(void*));
        node4->child_keys[i] = byte;
        node4->child_ptrs[i] = reinterpret_cast<Node*>(child);
        node->child_count++;
    }
    else
    {
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
        node16->child_ptrs[i] = reinterpret_cast<Node*>(child);
        return;
    }
    if (node->child_count < 16)
    {
        int mask = (1 << node->child_count) - 1;
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(byte),
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(node16->child_keys)));
        int bitfield = _mm_movemask_epi8(cmp);
        uint32_t idx;
        if (bitfield)
        {
            idx = __builtin_ctz(bitfield);
            memmove(&node16->child_keys[idx + 1], &node16->child_keys[idx], node->child_count - idx);
            memmove(&node16->child_ptrs[idx + 1], &node16->child_ptrs[idx], (node->child_count - idx) * sizeof(void*));
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
        node48->child_ptrs[node48->child_ptr_indexs[byte] - 1] = reinterpret_cast<Node*>(child);
        return;
    }
         
    if (node48->header.child_count < 48)
    {
        int pos = 0;
        while (node48->child_ptrs[pos]) pos++;
        node48->child_ptrs[pos] = reinterpret_cast<Node*>(child);
        node48->child_ptr_indexs[byte] = pos + 1;
        node48->header.child_count++;
    }
    else
    {
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
            memmove(&node->prefix[0], &node->prefix[0] + p + 1, node->prefix_length);
            assert(node->prefix_length < 7);

            Node* leafNode = makeProperNode(length);
            // 去掉第一个和最后一个，前缀6减去公共前缀长度就是分裂后的长度
            leafNode->prefix_length = 8 - depth - p - 2;
            memcpy(&leafNode->prefix[0], &key[depth + p + 1], leafNode->prefix_length);
            assert(leafNode->prefix_length < 7);
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
        insert(*next, next, key, length, val, depth+1);
    }
    else
    {
        Node* newNode = makeProperNode(length);
        assert(8 > depth - 2);
        memcpy(&newNode->prefix[0], &key[depth + 1], 8 - depth - 2);
        newNode->prefix_length = 8 - depth - 2;
        assert(newNode->prefix_length <= 8);
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

void AdaptiveRadixTree::Init(std::function<void(void*)> deref, std::function<void(void*)> incr)
{
    _root = reinterpret_cast<Node*>(makeNode4());
    _deref_func = deref;
    _incr_func = incr;
}

void* AdaptiveRadixTree::Search(uint64_t key)
{
    Node* node = _root;
    uint64_t reverse;
    unsigned char* data = reinterpret_cast<unsigned char*>(&reverse);
    unsigned char* rawdata = reinterpret_cast<unsigned char*>(&key);
    data[0] = rawdata[7];
    data[1] = rawdata[6];
    data[2] = rawdata[5];
    data[3] = rawdata[4];
    data[4] = rawdata[3];
    data[5] = rawdata[2];
    data[6] = rawdata[1];
    data[7] = rawdata[0];
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
    uint64_t reverse;
    unsigned char* data = reinterpret_cast<unsigned char*>(&reverse);
    unsigned char* rawdata = reinterpret_cast<unsigned char*>(&key);
    data[0] = rawdata[7];
    data[1] = rawdata[6];
    data[2] = rawdata[5];
    data[3] = rawdata[4];
    data[4] = rawdata[3];
    data[5] = rawdata[2];
    data[6] = rawdata[1];
    data[7] = rawdata[0];

    insert(_root, &_root, data, 1, val, 0);
}


// 需要保证[start, start + length]在同一个叶节点
void AdaptiveRadixTree::RangeInsert(uint64_t start, uint32_t length, void* val)
{
    assert(start % 256 + length <= 256);
    uint64_t reverse;
    unsigned char* data = reinterpret_cast<unsigned char*>(&reverse);
    unsigned char* rawdata = reinterpret_cast<unsigned char*>(&start);
    data[0] = rawdata[7];
    data[1] = rawdata[6];
    data[2] = rawdata[5];
    data[3] = rawdata[4];
    data[4] = rawdata[3];
    data[5] = rawdata[2];
    data[6] = rawdata[1];
    data[7] = rawdata[0];

    insert(_root, &_root, data, length, val, 0);
}

void AdaptiveRadixTree::RangeQuery(uint64_t start, uint32_t length, std::vector<void*>* vals)
{
    assert(start % 256 + length <= 256);
    Node* node = _root;
    uint64_t reverse;
    unsigned char* data = reinterpret_cast<unsigned char*>(&reverse);
    unsigned char* rawdata = reinterpret_cast<unsigned char*>(&start);
    data[0] = rawdata[7];
    data[1] = rawdata[6];
    data[2] = rawdata[5];
    data[3] = rawdata[4];
    data[4] = rawdata[3];
    data[5] = rawdata[2];
    data[6] = rawdata[1];
    data[7] = rawdata[0];
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

void AdaptiveRadixTree::destroy_node(Node* node, int depth)
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
                destroy_node(node4->child_ptrs[i], depth + node->prefix_length + 1);
            }
            break;
        }
        case NODE16:
        {
            Node16* node16 = reinterpret_cast<Node16*>(node);
            for (int i = 0; i < node->child_count; i++)
            {
                destroy_node(node16->child_ptrs[i], depth + node->prefix_length + 1);
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
                    destroy_node(node48->child_ptrs[i], depth + node->prefix_length + 1);
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
                    destroy_node(node256->child_ptrs[i], depth + node->prefix_length + 1);
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

    destroy_node(_root, 0);
}

}
