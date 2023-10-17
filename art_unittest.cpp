#define private public
#include "adaptive_radix_tree.h"
#define private private
#include "gtest/gtest.h"
#include "util.h"
#include <map>
#include <unordered_map>
#include <emmintrin.h>
#include <fcntl.h>

using namespace art;


static uint64_t makeKey(char* key)
{
    uint64_t k;
    memcpy(&k, key, 8);
    return k;
}

TEST(art, Simple)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr = (void*)rand();
    art->Insert(100UL, ptr);
    void* value = art->Search(100UL);
    EXPECT_EQ(value, ptr);

    art->Insert(1001UL, NULL);
    value = art->Search(1001ULL);
    EXPECT_EQ((uint64_t)value, NULL);
    EXPECT_EQ((uint64_t)art->Search(1002UL), NULL);

    art->Insert(1003UL, NULL);
    value = art->Search(1003ULL);
    EXPECT_EQ((uint64_t)value, NULL);

    art->Insert(1004UL, NULL);
    value = art->Search(1004UL);
    EXPECT_EQ((uint64_t)value, NULL);

    art->Insert(1005UL, NULL);
    value = art->Search(1005UL);
    EXPECT_EQ((uint64_t)value, NULL);

    ptr = (void*)rand();
    art->Insert(1111111111, ptr);
    EXPECT_EQ(art->Search(1111111111), ptr);

    ptr = (void*)rand();
    art->Insert(1111111112, ptr);
    EXPECT_EQ(art->Search(1111111112), ptr);

    art->Destroy();

    delete art;
}

TEST(art, Node4)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr  = (void*)rand();
    for (uint64_t i = 1000; i < 1004; i++)
    {
        art->Insert(i, ptr);
    }
    for (uint64_t i = 1000; i < 1004; i++)
    {
        void* value = art->Search(i);
        EXPECT_EQ(value, ptr);
    }

    art->Destroy();
    delete art;
}

TEST(art, Node16)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr  = (void*)rand();
    uint64_t start = (uint64_t)rand() << 32 + rand();
    for (uint64_t i = 0; i < 16; i++)
    {
        art->Insert(start + i, ptr);
    }
    for (uint64_t i = 0; i < 16; i++)
    {
        void* value = art->Search(start + i);
        EXPECT_EQ(value, ptr);
    }

    art->Destroy();
    delete art;
}

TEST(art, Node48)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr  = (void*)rand();
    uint64_t start = (uint64_t)rand() << 32 + rand();
    for (uint64_t i = 0; i < 48; i++)
    {
        art->Insert(start + i, ptr);
    }
    for (uint64_t i = 0; i < 48; i++)
    {
        void* value = art->Search(start + i);
        EXPECT_EQ(value, ptr);
    }

    art->Destroy();
    delete art;
}

TEST(art, Node256)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr  = (void*)rand();
    uint64_t start = (uint64_t)rand() << 32 + rand();
    for (uint64_t i = 0; i < 50; i++)
    {
        art->Insert(start + i, ptr);
    }
    for (uint64_t i = 0; i < 50; i++)
    {
        void* value = art->Search(start + i);
        EXPECT_EQ(value, ptr);
    }

    art->Destroy();
    delete art;
}

TEST(art, NodeSplit)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    uint64_t key1 = makeKey("aabcdef1");
    art->Insert(key1, &key1);

    uint64_t key2 = makeKey("aabcdef2");
    art->Insert(key2, &key2);

    uint64_t key3 = makeKey("aabcdef3");
    art->Insert(key3, &key3);

    uint64_t key4 = makeKey("aabchgi1");
    art->Insert(key4, &key4);

    EXPECT_EQ(art->Search(key1), &key1);
    EXPECT_EQ(art->Search(key2), &key2);
    EXPECT_EQ(art->Search(key3), &key3);
    EXPECT_EQ(art->Search(key4), &key4);

    art->Destroy();
    delete art;
}

TEST(art, Random48)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();
    std::map<uint64_t, void*> keyMap;
    void* val[1024];
    for (int i = 0; i < 1024; i++)
    {
        uint64_t key = (uint64_t)rand() << 32 + rand();
        art->Insert(key, val[i]);
        keyMap[key] = val[i];
    }

    for (std::map<uint64_t, void*>::iterator iter = keyMap.begin(); iter != keyMap.end(); iter++)
    {
        EXPECT_EQ(art->Search(iter->first), iter->second);
    }

    art->Destroy();
    delete art;
}

TEST(art, Node256_Reverse)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr = (void*)rand();
    uint64_t start = 1000;
    uint64_t end = 1004;
    for (uint64_t i = end; i >= start; i--)
    {
        art->Insert(i, ptr);
    }
    for (uint64_t i = start; i <= end; i++)
    {
        void* value = art->Search(i);
        EXPECT_EQ(value, ptr);
    }

    art->Destroy();
    delete art;
}

enum BenchMode
{
    SPARSE = 0,
    DENSE = 1
};

static void ArtBench(BenchMode mode, uint64_t keycount)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    std::vector<uint64_t> keys;
    if (mode == SPARSE)
    {
        keys.resize(keycount);
    }

    uint64_t insert_start = NowMicros();
    uint64_t key = 0;
    for (int i = 0; i < keycount; i++)
    {
        if (mode == SPARSE)
        {
            key = (uint64_t)rand() << 32 + rand();
        }
        else
        {
            key = (uint64_t)i;
        }
        art->Insert(key, (void*)rand());
        if (mode == SPARSE)
        {
            keys.push_back(key);
        }
    }

    uint64_t insert_end = NowMicros();
    for (int i = 0; i < keycount; i++)
    {
        if (mode == SPARSE)
        {
            key = keys[i];
        }
        else
        {
            key = (uint64_t)i;
        }
        art->Search(key);
    }

    uint64_t search_end = NowMicros();
    printf("%s pattern memory size per key %.2fB memory %ldB keys %ld insert %.2f lookup %.2f\n",
            mode == 0 ? "sparse" : "dense", art->MemoryUsage() / (float)keycount, art->MemoryUsage(), keycount,
            (insert_end - insert_start) / (float)keycount, (search_end - insert_end)/ (float)keycount);
    art->Destroy();
    delete art;
}

TEST(art, MemoryUsage)
{
    printf("sizeof(Node) %d sizeof(Node4) %d sizeof(Node16) %d sizeof(Node48) %d sizeof(Node256) %d\n",
            sizeof(Node), sizeof(Node4), sizeof(Node16), sizeof(Node48), sizeof(Node256));
    ArtBench(SPARSE, 10000000);
    ArtBench(DENSE, 10000000);
    ArtBench(SPARSE, 1000000);
    ArtBench(DENSE, 1000000);
    ArtBench(SPARSE, 10000);
    ArtBench(DENSE, 10000);
}

static void ArtRangeBench(BenchMode mode, uint64_t rangecount, uint32_t range)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    void* ptr = (void*)rand();
    uint64_t insert_start = NowMicros();
    uint64_t start = 0;
    std::vector<uint64_t> ranges;
    if (mode == SPARSE)
    {
        ranges.resize(rangecount);
    }
    for (uint64_t i = 0; i < rangecount; i++)
    {
        if (mode == SPARSE)
        {
            start = (uint64_t)rand() * range;
            ranges[i] = start;
        }
        else
        {
            start = i * range;
        }
        art->RangeInsert(start, range, ptr);
    }
    uint64_t insert_end = NowMicros();
    std::vector<void*> vec;
    for (int i = 0; i < rangecount; i++)
    {
        if (mode == SPARSE)
        {
            start = ranges[i];
        }
        else
        {
            start = i * range;
        }
        art->RangeQuery(start, range, &vec);
        vec.clear();
    }
    uint64_t query_end = NowMicros();

    printf("%s pattern memory size per key %.2fB memory %ldB rangecount %ld range %d range insert %.2f range lookup %.2f\n",
            mode == SPARSE ? "sparse" : "dense", art->MemoryUsage() / ((float)rangecount * range), art->MemoryUsage(), rangecount,
            range, (insert_end - insert_start) / (float)rangecount, (query_end - insert_end)/ (float)rangecount);
    art->Destroy();
    delete art;
}

TEST(art, MemoryUsage_Range)
{
    printf("sizeof(Node) %d sizeof(Node4) %d sizeof(Node16) %d sizeof(Node48) %d sizeof(Node256) %d\n",
            sizeof(Node), sizeof(Node4), sizeof(Node16), sizeof(Node48), sizeof(Node256));

    ArtRangeBench(SPARSE, 10000000 / 4, 4);
    ArtRangeBench(DENSE, 10000000 / 4, 4);
    ArtRangeBench(SPARSE, 100000 / 4, 4);
    ArtRangeBench(DENSE, 100000 / 4, 4);

    ArtRangeBench(SPARSE, 10000000 / 32, 32);
    ArtRangeBench(DENSE, 10000000 / 32, 32);
    ArtRangeBench(SPARSE, 100000 / 32, 32);
    ArtRangeBench(DENSE, 100000 / 32, 32);

    ArtRangeBench(SPARSE, 10000000 / 128, 128);
    ArtRangeBench(DENSE, 10000000 / 128, 128);
    ArtRangeBench(SPARSE, 100000 / 128, 128);
    ArtRangeBench(DENSE, 100000 / 128, 128);
}

uint64_t MemoryUsage()
{
    char path[64];
    sprintf(path, "/proc/%d/status", getpid());
    FILE* file = fopen(path, "r");
    char* line;
    size_t len = 0;
    do
    {
        int ret = getline(&line, &len, file);
    } while (memcmp(line, "VmSize:", 7) != 0);
    
    printf("%s\n", line);
    return 0;
}

// TEST(art, DISABLED_MemoryUsage_rb)
// {
//     std::vector<uint64_t> keys;
//     std::map<uint64_t, void*> rb;
//     std::unordered_map<uint64_t, void*> hash;
//     MemoryUsage();
//     uint64_t start = NowMicros();
//     for (int i = 0; i < 10000000; i++)
//     {
//         uint64_t key = (uint64_t)rand() << 32 | rand(); 
//         rb[key] = nullptr;
//         keys.push_back(key);
//     }
//     uint64_t end = NowMicros();
//     MemoryUsage();
//     printf("insert %.2f\n", (end - start) / 10000000.0);
// }

TEST(art, LeafNode_Insert4)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node4* node4 = new Node4;
    Node* node = reinterpret_cast<Node*>(node4);

    // [0]
    art->addLeafChild4(node, &node, 0, 1, nullptr);
    EXPECT_EQ(node->child_count, 1);
    EXPECT_EQ(node4->child_keys[0], 0);

    // [0]
    art->addLeafChild4(node, &node, 0, 1, nullptr);
    EXPECT_EQ(node->child_count, 1);
    EXPECT_EQ(node4->child_keys[0], 0);

    // [0] insert [0,1,2] -> [0,1,2]
    art->addLeafChild4(node, &node, 0, 3, nullptr);

    EXPECT_EQ(node->child_count, 3);
    EXPECT_EQ(node4->child_keys[0], 0);
    EXPECT_EQ(node4->child_keys[1], 1);
    EXPECT_EQ(node4->child_keys[2], 2);

    new (node4) Node4;
    // [255]
    art->addLeafChild4(node, &node, 255, 1, nullptr);
    // [1,255]
    art->addLeafChild4(node, &node, 1, 1, nullptr);
    // [1, 255] insert [0,1,2] -> [0,1,2,255]
    art->addLeafChild4(node, &node, 0, 3, nullptr);
    EXPECT_EQ(node->child_count, 4);
    EXPECT_EQ(node4->child_keys[0], 0);
    EXPECT_EQ(node4->child_keys[1], 1);
    EXPECT_EQ(node4->child_keys[2], 2);
    EXPECT_EQ(node4->child_keys[3], 255);

    new (node4) Node4;
    // [12]
    art->addLeafChild4(node, &node, 12, 1, nullptr);
    // [12] insert [15] -> [12,15]
    art->addLeafChild4(node, &node, 15, 1, nullptr);
    // [12,15] insert [12,13,14,15] -> [12,13,14,15]
    art->addLeafChild4(node, &node, 12, 4, nullptr);
    EXPECT_EQ(node->child_count, 4);
    EXPECT_EQ(node4->child_keys[0], 12);
    EXPECT_EQ(node4->child_keys[1], 13);
    EXPECT_EQ(node4->child_keys[2], 14);
    EXPECT_EQ(node4->child_keys[3], 15);

    new (node) Node4;

    // [0]
    art->addLeafChild4(node, &node, 0, 1, nullptr);
    // [0,255]
    art->addLeafChild4(node, &node, 255, 1, nullptr);
    // [0,48,49,255]
    art->addLeafChild4(node, &node, 48, 2, nullptr);
    EXPECT_EQ(node->child_count, 4);
    EXPECT_EQ(node4->child_keys[0], 0);
    EXPECT_EQ(node4->child_keys[1], 48);
    EXPECT_EQ(node4->child_keys[2], 49);
    EXPECT_EQ(node4->child_keys[3], 255);

    new (node) Node4;

    // [10]
    art->addLeafChild4(node, &node, 10, 1, nullptr);
    // [10,11]
    art->addLeafChild4(node, &node, 11, 1, nullptr);
    // [9,10,11,12]
    art->addLeafChild4(node, &node, 9, 4, nullptr);
    EXPECT_EQ(node->child_count, 4);
    EXPECT_EQ(node4->child_keys[0], 9);
    EXPECT_EQ(node4->child_keys[1], 10);
    EXPECT_EQ(node4->child_keys[2], 11);
    EXPECT_EQ(node4->child_keys[3], 12);

    art->Destroy();
    delete art;
}

TEST(art, LeafNode_Expand_4_16)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node4* node4 = new Node4;
    art->addChild4(node4, NULL, 10, (void*)10);
    art->addChild4(node4, NULL, 34, (void*)34);
    art->addChild4(node4, NULL, 222, (void*)222);
    Node* node = art->expandLeafChild(reinterpret_cast<Node*>(node4), 16);
    EXPECT_EQ(node->type, NODE16);
    EXPECT_EQ(*art->findChild(node, 10), (void*)10);
    EXPECT_EQ(*art->findChild(node, 34), (void*)34);
    EXPECT_EQ(*art->findChild(node, 222), (void*)222);
    delete node;

    art->Destroy();
    delete art;
}

TEST(art, LeafNode_Expand_4_48)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node4* node4 = new Node4;
    art->addChild4(node4, NULL, 10, (void*)10);
    art->addChild4(node4, NULL, 34, (void*)34);
    art->addChild4(node4, NULL, 222, (void*)222);
    Node* node = art->expandLeafChild(reinterpret_cast<Node*>(node4), 32);
    EXPECT_EQ(node->type, NODE48);
    EXPECT_EQ(*art->findChild(node, 10), (void*)10);
    EXPECT_EQ(*art->findChild(node, 34), (void*)34);
    EXPECT_EQ(*art->findChild(node, 222), (void*)222);
    delete node;

    art->Destroy();
    delete art;
}

TEST(art, LeafNode_Expand_4_256)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node4* node4 = new Node4;
    art->addChild4(node4, NULL, 10, (void*)10);
    art->addChild4(node4, NULL, 34, (void*)34);
    art->addChild4(node4, NULL, 222, (void*)222);
    Node* node = art->expandLeafChild(reinterpret_cast<Node*>(node4), 200);
    EXPECT_EQ(node->type, NODE256);
    EXPECT_EQ(*art->findChild(node, 10), (void*)10);
    EXPECT_EQ(*art->findChild(node, 34), (void*)34);
    EXPECT_EQ(*art->findChild(node, 222), (void*)222);
    delete node;

    art->Destroy();
    delete art;
}

TEST(art, LeafNode_Expand_16_48)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node16* node16 = new Node16;
    art->addChild16(node16, NULL, 10, (void*)10);
    art->addChild16(node16, NULL, 34, (void*)34);
    art->addChild16(node16, NULL, 222, (void*)222);
    Node* node = art->expandLeafChild(reinterpret_cast<Node*>(node16), 30);
    EXPECT_EQ(node->type, NODE48);
    EXPECT_EQ(*art->findChild(node, 10), (void*)10);
    EXPECT_EQ(*art->findChild(node, 34), (void*)34);
    EXPECT_EQ(*art->findChild(node, 222), (void*)222);
    delete node;

    art->Destroy();
    delete art;
}

TEST(art, LeafNode_Expand_16_256)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node16* node16 = new Node16;
    art->addChild16(node16, NULL, 10, (void*)10);
    art->addChild16(node16, NULL, 34, (void*)34);
    art->addChild16(node16, NULL, 222, (void*)222);
    art->addChild16(node16, NULL, 230, (void*)230);
    art->addChild16(node16, NULL, 254, (void*)254);
    Node* node = art->expandLeafChild(reinterpret_cast<Node*>(node16), 250);
    EXPECT_EQ(node->type, NODE256);
    EXPECT_EQ(*art->findChild(node, 10), (void*)10);
    EXPECT_EQ(*art->findChild(node, 34), (void*)34);
    EXPECT_EQ(*art->findChild(node, 222), (void*)222);
    EXPECT_EQ(*art->findChild(node, 230), (void*)230);
    EXPECT_EQ(*art->findChild(node, 254), (void*)254);
    delete node;

    art->Destroy();
    delete art;
}

TEST(art, LeafNode_Expand_48_256)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node48* node48 = new Node48;
    art->addChild48(node48, NULL, 10, (void*)10);
    art->addChild48(node48, NULL, 34, (void*)34);
    art->addChild48(node48, NULL, 35, (void*)35);
    art->addChild48(node48, NULL, 36, (void*)36);
    art->addChild48(node48, NULL, 200, (void*)200);
    art->addChild48(node48, NULL, 254, (void*)254);
    art->addChild48(node48, NULL, 255, (void*)255);
    Node* node = art->expandLeafChild(reinterpret_cast<Node*>(node48), 49);
    EXPECT_EQ(node->type, NODE256);
    EXPECT_EQ(*art->findChild(node, 10), (void*)10);
    EXPECT_EQ(*art->findChild(node, 34), (void*)34);
    EXPECT_EQ(*art->findChild(node, 35), (void*)35);
    EXPECT_EQ(*art->findChild(node, 36), (void*)36);
    EXPECT_EQ(*art->findChild(node, 200), (void*)200);
    EXPECT_EQ(*art->findChild(node, 254), (void*)254);
    EXPECT_EQ(*art->findChild(node, 255), (void*)255);
    delete node;

    art->Destroy();
    delete art;
}

static bool checkChild(AdaptiveRadixTree* art, Node* node, uint8_t start, uint8_t length, void* expected)
{
    for (int i = 0; i < length; i++)
    {
        EXPECT_EQ(*art->findChild(node, start + i), expected);
    }
}

TEST(art, DISABLED_LeafNode_Leaf16_Insert)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    Node16* node16 = new Node16;
    Node* node = reinterpret_cast<Node*>(node16);
    art->addLeafChild16(node16, 0, 16, nullptr);
    checkChild(art, node, 0, 16, nullptr);

    // 前面覆盖后面
    // [0, 15] + [4-12] ->[0, 4-12, 15]
    new (node16) Node16;
    art->addLeafChild16(node16, 0, 1, (void*)0);
    art->addLeafChild16(node16, 15, 1, (void*)15);
    art->addLeafChild16(node16, 4, 9, (void*)4);

    checkChild(art, node, 0, 1, (void*)0);
    checkChild(art, node, 4, 9, (void*)4);
    checkChild(art, node, 15, 1, (void*)15);

    // 前面overlap
    // [0,1,2] + [1,2,3,4,5] -> [0,1,2,3,4,5]
    new (node16) Node16;
    art->addLeafChild16(node16, 0, 3, (void*)0);
    art->addLeafChild16(node16, 1, 5, (void*)1);

    checkChild(art, node, 0, 1, (void*)0);
    checkChild(art, node, 1, 5, (void*)1);

    // 后面overlap
    // [248,249,250,251,252,253] + [246,247,248,249,250,251]  -> [246->253]
    new (node16) Node16;
    art->addLeafChild16(node16, 248, 6, (void*)0);
    art->addLeafChild16(node16, 246, 6, (void*)1);

    checkChild(art, node, 246, 6, (void*)1);
    checkChild(art, node, 252, 2, (void*)0);

    // 后面覆盖前面
    // [38-42] + [30-45] -> [30-45]
    new (node16) Node16;
    art->addLeafChild16(node16, 38, 2, (void*)1122);
    art->addLeafChild16(node16, 30, 16, (void*)2211);

    checkChild(art, node, 30, 16, (void*)2211);

    // 后面覆盖前面
    // [55] + [50-60] -> [50-60]
    new (node16) Node16;
    art->addLeafChild16(node16, 55, 1, (void*)1);
    art->addLeafChild16(node16, 50, 10, (void*)2);

    checkChild(art, node, 50, 10, (void*)2);

    // one by one
    // [0] + [1] + [2] + [3] -> [0,1,2,3]
    new (node16) Node16;
    art->addLeafChild16(node16, 0, 1, (void*)1);
    art->addLeafChild16(node16, 1, 1, (void*)2);
    art->addLeafChild16(node16, 2, 1, (void*)3);
    art->addLeafChild16(node16, 3, 1, (void*)4);

    checkChild(art, node, 0, 1, (void*)1);
    checkChild(art, node, 1, 1, (void*)2);
    checkChild(art, node, 2, 1, (void*)3);
    checkChild(art, node, 3, 1, (void*)4);

    // one by one
    // [0] + [5] + [7] + [9] -> [0,5,7,9]
    new (node16) Node16;
    art->addLeafChild16(node16, 0, 1, (void*)1);
    art->addLeafChild16(node16, 5, 1, (void*)2);
    art->addLeafChild16(node16, 7, 1, (void*)3);
    art->addLeafChild16(node16, 9, 1, (void*)4);

    checkChild(art, node, 0, 1, (void*)1);
    checkChild(art, node, 5, 1, (void*)2);
    checkChild(art, node, 7, 1, (void*)3);
    checkChild(art, node, 9, 1, (void*)4);

    new (node16) Node16;
    art->addLeafChild16(node16, (unsigned char)0, 2, (void*)1);
    art->addLeafChild16(node16, (unsigned char)115, 14, (void*)2);

    art->Destroy();
    delete art;
}

static bool checkValueRange(AdaptiveRadixTree* art, uint64_t start, uint8_t length, void* expect_val, std::vector<void*>* expect_vals = NULL)
{
    std::vector<void*> vals;
    art->RangeQuery(start, length, &vals);
    EXPECT_EQ(vals.size(), (size_t)length);

    for (int i = 0; i < length; i++)
    {
        if (expect_vals != NULL)
        {
            EXPECT_EQ(vals[i], (*expect_vals)[i]);
        }
        else
        {
            EXPECT_EQ(vals[i], expect_val);
        }
    }
}

TEST(art, DISABLED_RangeInsert_Basic)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    art->RangeInsert(0ULL, 32, (void*)12345);
    checkValueRange(art, 0, 32, (void*)12345);

    art->RangeInsert(32ULL, 224, (void*)54321);
    checkValueRange(art, 0, 32, (void*)12345);
    checkValueRange(art, 32, 224, (void*)54321);

    art->RangeInsert(0ULL, 1, (void*)0);
    checkValueRange(art, 0, 1, (void*)0);
    checkValueRange(art, 1, 31, (void*)12345);
    checkValueRange(art, 32, 224, (void*)54321);

    art->RangeInsert(1ULL, 1, (void*)0);
    checkValueRange(art, 0, 1, (void*)0);
    checkValueRange(art, 1, 1, (void*)0);
    checkValueRange(art, 2, 30, (void*)12345);
    checkValueRange(art, 32, 224, (void*)54321);

    art->Destroy();
    delete art;
}

TEST(art, RangeInsert_Basic2)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    for (uint64_t i = 0; i < 256; i++)
    {
        art->RangeInsert(i, 1, (void*)12345);
    }
    checkValueRange(art, 0, 256, (void*)12345);

    art->RangeInsert(0, 256, (void*)11111);
    for (uint64_t i = 0; i < 256; i++)
    {
        checkValueRange(art, i, 1, (void*)11111);
    }

    art->RangeInsert(0, 120, (void*)22222);
    art->RangeInsert(60, 120, (void*)33333);
    checkValueRange(art, 0, 60, (void*)22222);
    checkValueRange(art, 60, 120, (void*)33333);

    art->Destroy();
    delete art;
}

TEST(art, Random)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    uint64_t keycount = 10000000;
    std::map<uint64_t, void*> verifyMap;
    uint64_t key = 0;

    do
    {
        void* ptr = (void*)rand();
        key = (uint64_t)rand() << 32 + rand();
        art->Insert(key, ptr);
        verifyMap[key] = ptr;
        keycount--;
    } while (keycount > 0);
    

    for (std::map<uint64_t, void*>::iterator iter = verifyMap.begin(); iter != verifyMap.end(); iter++)
    {
        void* value = art->Search(iter->first);
        char* key = (char*)&iter->first;
        char arr[32];
        sprintf(arr, "%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu\n", key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
        EXPECT_EQ(value, iter->second) << "key: " << arr;
    }
    art->Destroy();
    delete art;
}

TEST(art, RangeInsert_Random)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    uint64_t rangecount = 100000;
    std::map<uint64_t, void*> verifyMap;
    uint64_t verifyCursor = 0;

    do
    {
        uint64_t start = (uint64_t)rand() << 32 + rand();
        uint32_t lengthmax = 256 - start % 256;
        uint32_t length = std::max(1U, rand() % lengthmax);
        void* ptr = (void*)rand();

        art->RangeInsert(start, length, ptr);
        for (int i = 0; i < length; i++)
        {
            verifyMap[start + i] = ptr;
        }
        rangecount--;
        if (rangecount % 1000 == 0)
        {
            uint64_t verifyCount = 0;
            for (std::map<uint64_t, void*>::iterator iter = verifyMap.upper_bound(verifyCursor); iter != verifyMap.end(); iter++)
            {
                checkValueRange(art, iter->first, 1, iter->second);
                verifyCount++;
                if (verifyCount > 1000)
                {
                    verifyCursor = iter->first;
                    break;
                }
            }
        }
    } while (rangecount > 0);
    printf("Memory usage per key %dB\n", art->MemoryUsage() / verifyMap.size());
    art->Destroy();
    delete art;
}

TEST(art, Serialization_Node4)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;

    Node4* n4 = art->makeNode4();

    n4->header.child_count = 4;
    n4->header.is_leaf = false;
    n4->header.prefix[0] = 'a';
    n4->header.type = NODE4;
    n4->child_keys[0] = 0;
    n4->child_keys[1] = 1;
    n4->child_keys[2] = 2;
    n4->child_keys[3] = 3;

    char* buf = new char[4096];
    int nodeSize = 0;

    EXPECT_TRUE(art->serializationNode(reinterpret_cast<Node*>(n4), buf, nodeSize));
    EXPECT_TRUE(nodeSize > 0);

    Node* denode;
    EXPECT_TRUE(art->deserializationNode(&denode, &buf));

    EXPECT_EQ(memcmp(n4, denode, sizeof(Node4)), 0);
}

TEST(art, Serialization_Node16)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;

    Node16* n16 = art->makeNode16();

    n16->header.child_count = 12;
    n16->header.is_leaf = false;
    n16->header.prefix[0] = 'a';
    n16->header.type = NODE16;
    n16->child_keys[0] = 0;
    n16->child_keys[1] = 1;
    n16->child_keys[2] = 2;
    n16->child_keys[3] = 3;
    memset(&n16->child_ptrs[0], 0, sizeof(void*) * 16);

    char* buf = new char[4096];
    int nodeSize = 0;

    EXPECT_TRUE(art->serializationNode(reinterpret_cast<Node*>(n16), buf, nodeSize));
    EXPECT_TRUE(nodeSize > 0);

    Node* denode;
    EXPECT_TRUE(art->deserializationNode(&denode, &buf));

    EXPECT_EQ(memcmp(n16, denode, sizeof(Node16)), 0);
}

TEST(art, Serialization_Node48)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;

    Node48* n48 = art->makeNode48();

    n48->header.child_count = 5;
    n48->header.is_leaf = false;
    n48->header.prefix[0] = 'a';
    n48->header.type = NODE48;
    n48->child_ptr_indexs[0] = 0;
    n48->child_ptr_indexs[1] = 1;
    n48->child_ptr_indexs[2] = 2;
    n48->child_ptr_indexs[8] = 3;
    n48->child_ptr_indexs[12] = 133;
    memset(&n48->child_ptrs[0], 0, sizeof(void*) * 48);

    char* buf = new char[4096];
    int nodeSize = 0;

    EXPECT_TRUE(art->serializationNode(reinterpret_cast<Node*>(n48), buf, nodeSize));
    EXPECT_TRUE(nodeSize > 0);

    Node* denode;
    EXPECT_TRUE(art->deserializationNode(&denode, &buf));

    EXPECT_EQ(memcmp(n48, denode, sizeof(Node48)), 0);
}

TEST(art, Serialization_Node256)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;

    Node256* n256 = art->makeNode256();

    n256->header.child_count = 5;
    n256->header.is_leaf = false;
    n256->header.prefix[0] = 'a';
    n256->header.type = NODE256;
    memset(&n256->child_ptrs[0], 0, sizeof(void*) * 256);
    n256->child_ptrs[100] = (Node*)(void*)1;

    char* buf = new char[4096];
    int nodeSize = 0;

    EXPECT_TRUE(art->serializationNode(reinterpret_cast<Node*>(n256), buf, nodeSize));
    EXPECT_TRUE(nodeSize > 0);

    Node* denode;
    EXPECT_TRUE(art->deserializationNode(&denode, &buf));
    for (int i = 0; i < 256; i++)
    {
        if (i == 100)
        {
            EXPECT_TRUE(reinterpret_cast<Node256*>(denode)->child_bitmap->bitmap[i] == 1);
        }
        else
        {
            EXPECT_TRUE(reinterpret_cast<Node256*>(denode)->child_bitmap->bitmap[i] == 0);
        }
    }
    n256->child_ptrs[100] = NULL;
    reinterpret_cast<Node256*>(denode)->child_bitmap = NULL;

    EXPECT_EQ(memcmp(n256, denode, sizeof(Node256)), 0);
}

TEST(art, Serialization_Tree)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    std::map<uint64_t, void*> verifyMap;
    int ranges = 10000;

    while (ranges-- > 0)
    {
        uint64_t start = (uint64_t)rand() << 32 + rand();
        uint32_t lengthmax = 256 - start % 256;
        uint32_t length = std::max(1U, rand() % lengthmax);
        void* ptr = (void*)rand();

        art->RangeInsert(start, length, ptr);
        for (int i = 0; i < length; i++)
        {
            verifyMap[start + i] = ptr;
        }
    }

    void* buf = NULL;
    int bufSize = 0;
    art->Serialization(&buf, bufSize);

    EXPECT_TRUE(buf != NULL);

    art->Destroy();
    delete art;

    AdaptiveRadixTree* newArt = new AdaptiveRadixTree;
    newArt->Deserialization(buf, bufSize);

    free(buf);

    newArt->DumpTree();

    for (auto it = verifyMap.begin(); it != verifyMap.end(); it++) {
        checkValueRange(newArt, it->first, 1, it->second);
    }

    newArt->Destroy();
    delete newArt;
}

TEST(art, Serialization_Tree_2_File)
{
    AdaptiveRadixTree* art = new AdaptiveRadixTree;
    art->Init();

    std::map<uint64_t, void*> verifyMap;
    int ranges = 10000;

    while (ranges-- > 0)
    {
        uint64_t start = (uint64_t)rand() << 32 + rand();
        uint32_t lengthmax = 256 - start % 256;
        uint32_t length = std::max(1U, rand() % lengthmax);
        void* ptr = (void*)rand();

        art->RangeInsert(start, length, ptr);
        for (int i = 0; i < length; i++)
        {
            verifyMap[start + i] = ptr;
        }
    }

    void* buf = NULL;
    int bufSize = 0;
    art->Serialization(&buf, bufSize);

    EXPECT_TRUE(buf != NULL);

    art->Destroy();
    delete art;

    int fd = open("./art_dump", O_WRONLY|O_CREAT);
    assert(fd > 0);
    write(fd, buf, bufSize);
    close(fd);
    free(buf);

    char* newbuf = new char[bufSize];
    fd = open("./art_dump", O_RDONLY);
    read(fd, newbuf, bufSize);
    unlink("./art_dump");

    AdaptiveRadixTree* newArt = new AdaptiveRadixTree;
    newArt->Deserialization(newbuf, bufSize);

    newArt->DumpTree();
    printf("total serialize size %dB keys %d memory used per key %dB\n", bufSize, verifyMap.size(), bufSize / verifyMap.size());

    for (auto it = verifyMap.begin(); it != verifyMap.end(); it++) {
        checkValueRange(newArt, it->first, 1, it->second);
    }

    newArt->Destroy();
    delete newArt;
}

TEST(art, SSE_lt)
{
    int mask = (1 << 16) - 1;
    signed char array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array));
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(0), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 1);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array));
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(14), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 15);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array));
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(15), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, -1);
    }


    unsigned char array1[] = {120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135};

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array1));
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(120), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 1);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array1));
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(127), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, -1);
    }
}

TEST(art, SSE_gt)
{
    int mask = (1 << 16) - 1;
    signed char array[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array));
        __m128i cmp = _mm_cmpgt_epi8(_mm_set1_epi8(0), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, -1);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array));
        __m128i cmp = _mm_cmpgt_epi8(_mm_set1_epi8(14), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 0);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array));
        __m128i cmp = _mm_cmpgt_epi8(_mm_set1_epi8(15), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 0);
    }


    unsigned char array1[] = {120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135};
    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array1));
        __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(120), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 1);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array1));
        __m128i cmp = _mm_cmpgt_epi8(_mm_set1_epi8(127), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 0);
    }

    {
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(array1));
        __m128i cmp = _mm_cmpgt_epi8(_mm_set1_epi8(135), b);
        int bitfieldend = _mm_movemask_epi8(cmp) & mask;
        int index = -1;
        if (bitfieldend)
        {
            index = __builtin_ctz(bitfieldend);
        }
        EXPECT_EQ(index, 8);
    }
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
