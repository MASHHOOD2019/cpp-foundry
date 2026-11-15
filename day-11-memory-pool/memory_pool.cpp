// Author: Mashhood Ali
// Day 11 - Memory Pool (C++)
// A fixed-size block allocator. Instead of calling new/delete
// for thousands of small, same-sized objects (slow, and it
// fragments the heap), the pool grabs memory in big chunks and
// hands out blocks from a free list. This is the allocator
// behind game engines, network servers and embedded systems:
// predictable, cache-friendly, and fragmentation-free.
//
// Approach: each chunk holds N blocks of the same size, aligned
// for any type. Free blocks form an intrusive linked list, so
// allocate() pops the head and deallocate() pushes a block back,
// both in O(1) time with no extra bookkeeping memory. When the
// free list is empty, a new chunk is added. A mutex guards the
// free list, so the pool is safe to share across threads.
// Time: O(1) allocate / deallocate, Space: O(blocks in use).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>
using namespace std;

class MemoryPool
{
public:
    // blockSize: bytes per block, blocksPerChunk: blocks in each chunk.
    MemoryPool(size_t blockSize, size_t blocksPerChunk = 64)
        : blockSize_(max(blockSize, sizeof(void*))),
          blocksPerChunk_(blocksPerChunk),
          freeList_(nullptr), totalBlocks_(0), freeBlocks_(0)
    {
        if (blockSize == 0 || blocksPerChunk == 0)
            throw invalid_argument("block size and blocks per chunk must be > 0");
        addChunk();
    }

    ~MemoryPool()
    {
        for (void* chunk : chunks_)
            operator delete[](chunk);
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Returns a block of blockSize_ bytes, or nullptr if out of memory.
    void* allocate()
    {
        lock_guard<mutex> lock(mutex_);
        if (freeList_ == nullptr)
            addChunk();
        if (freeList_ == nullptr)
            return nullptr;
        void* block = freeList_;
        freeList_ = *static_cast<void**>(freeList_);
        --freeBlocks_;
        return block;
    }

    // Returns a block to the pool. Pointers from outside the pool
    // are ignored instead of corrupting the free list.
    void deallocate(void* ptr)
    {
        if (ptr == nullptr)
            return;
        lock_guard<mutex> lock(mutex_);
        if (!owns(ptr))
            return;
        *static_cast<void**>(ptr) = freeList_;
        freeList_ = ptr;
        ++freeBlocks_;
    }

    size_t blockSize() const { return blockSize_; }
    size_t totalBlocks() const { return totalBlocks_; }
    size_t freeBlocks() const { return freeBlocks_; }
    size_t usedBlocks() const { return totalBlocks_ - freeBlocks_; }
    size_t chunks() const { return chunks_.size(); }

private:
    void addChunk()
    {
        size_t bytes = blockSize_ * blocksPerChunk_;
        char* chunk = static_cast<char*>(operator new[](bytes));
        chunks_.push_back(chunk);
        // Link the new blocks into the free list, last block first.
        for (size_t i = 0; i < blocksPerChunk_; ++i)
        {
            char* block = chunk + i * blockSize_;
            *reinterpret_cast<void**>(block) =
                (i + 1 < blocksPerChunk_) ? block + blockSize_ : freeList_;
        }
        freeList_ = chunk;
        totalBlocks_ += blocksPerChunk_;
        freeBlocks_ += blocksPerChunk_;
    }

    bool owns(void* ptr) const
    {
        for (void* chunk : chunks_)
        {
            char* start = static_cast<char*>(chunk);
            char* end = start + blockSize_ * blocksPerChunk_;
            if (ptr >= start && ptr < end)
            {
                size_t offset = static_cast<char*>(ptr) - start;
                return offset % blockSize_ == 0;
            }
        }
        return false;
    }

    size_t blockSize_;
    size_t blocksPerChunk_;
    void* freeList_;
    size_t totalBlocks_;
    size_t freeBlocks_;
    vector<void*> chunks_;
    mutex mutex_;
};

static bool check(const char* name, bool ok)
{
    cout << name << ": " << (ok ? "OK" : "FAILED") << '\n';
    return ok;
}

int main()
{
    bool allOk = true;

    // Check 1: allocate 100 blocks, write and read back values.
    MemoryPool pool(sizeof(int), 64);
    vector<int*> ptrs;
    for (int i = 0; i < 100; ++i)
    {
        int* p = static_cast<int*>(pool.allocate());
        if (p == nullptr) { allOk = false; break; }
        *p = i;
        ptrs.push_back(p);
    }
    long sum = 0;
    for (int* p : ptrs)
        sum += *p;
    allOk &= check("100 allocations, sum 0..99", sum == 4950);
    allOk &= check("grew past one chunk", pool.chunks() == 2);
    allOk &= check("used blocks counted", pool.usedBlocks() == 100);

    // Check 2: free every second block, reallocate, values still work.
    vector<void*> freed;
    for (size_t i = 0; i < ptrs.size(); i += 2)
    {
        freed.push_back(ptrs[i]);
        pool.deallocate(ptrs[i]);
    }
    allOk &= check("50 blocks freed", pool.usedBlocks() == 50);
    vector<int*> again;
    for (size_t i = 0; i < freed.size(); ++i)
    {
        int* p = static_cast<int*>(pool.allocate());
        *p = 1000 + static_cast<int>(i);
        again.push_back(p);
    }
    long sum2 = 0;
    for (int* p : again)
        sum2 += *p;
    allOk &= check("50 reallocations reuse freed blocks",
                   sum2 == 50 * 1000 + 1225 && pool.chunks() == 2);

    // Check 3: foreign and null pointers are safely ignored.
    int stackValue = 42;
    size_t usedBefore = pool.usedBlocks();
    pool.deallocate(&stackValue);
    pool.deallocate(nullptr);
    allOk &= check("foreign/null deallocate ignored",
                   pool.usedBlocks() == usedBefore);

    // Check 4: pool of a bigger type, allocate until a third chunk.
    MemoryPool bigPool(256, 16);
    vector<void*> bigPtrs;
    for (int i = 0; i < 40; ++i)
        bigPtrs.push_back(bigPool.allocate());
    allOk &= check("256-byte blocks, 40 allocs over 3 chunks",
                   bigPool.chunks() == 3 && bigPool.usedBlocks() == 40);
    for (void* p : bigPtrs)
        bigPool.deallocate(p);
    allOk &= check("all big blocks returned",
                   bigPool.usedBlocks() == 0 && bigPool.freeBlocks() == 48);

    cout << (allOk ? "All checks passed.\n" : "Some checks FAILED.\n");
    return allOk ? 0 : 1;
}
