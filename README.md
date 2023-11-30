# Adaptive Radix Tree

https://db.in.tum.de/~leis/papers/ART.pdf


1. support range insert/range query，must aligned in 256 keys
2. super fast
3. super low memory cost
4. support serialization and deserialization

key is fixed 8 bytes，value is 8 bytes


### kv api

void RangeInsert(uint64_t start, uint32_t length, void* val);

void RangeQuery(uint64_t start, uint32_t length, std::vector<void*>* vals);

### block api

void RangeInsert(LbaRange range, Location location);

void RangeQuery(LbaRange range, std::vector<Location>* locations);

