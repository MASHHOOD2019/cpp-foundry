// Author: Mashhood Ali
// Day 5 - LRU Cache (C++)
// A fixed-capacity cache that evicts the least recently used item.
// Classic interview / systems problem (LeetCode 146).
// Example: capacity 2, put(1,1), put(2,2), get(1) -> 1,
// put(3,3) evicts key 2, get(2) -> -1, get(3) -> 3.
//
// Approach: a doubly linked list holds keys in use order
// (most recent at the front) and a hash map points at each
// list node, so get and put both run in O(1) time.
// Time: O(1) per get / put, Space: O(capacity).

#include <iostream>
#include <list>
#include <unordered_map>
using namespace std;

class LRUCache
{
public:
    LRUCache(int capacity) : capacity(capacity) {}

    int get(int key)
    {
        auto it = cache.find(key);
        if (it == cache.end())
        {
            return -1;
        }
        order.splice(order.begin(), order, it->second);
        return it->second->second;
    }

    void put(int key, int value)
    {
        auto it = cache.find(key);
        if (it != cache.end())
        {
            it->second->second = value;
            order.splice(order.begin(), order, it->second);
            return;
        }
        if ((int)order.size() == capacity)
        {
            int lruKey = order.back().first;
            cache.erase(lruKey);
            order.pop_back();
        }
        order.push_front({key, value});
        cache[key] = order.begin();
    }

private:
    int capacity;
    list<pair<int, int>> order;
    unordered_map<int, list<pair<int, int>>::iterator> cache;
};

int main()
{
    LRUCache cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    cout << "get(1): " << cache.get(1) << endl;
    cache.put(3, 3);
    cout << "get(2): " << cache.get(2) << endl;
    cache.put(4, 4);
    cout << "get(1): " << cache.get(1) << endl;
    cout << "get(3): " << cache.get(3) << endl;
    cout << "get(4): " << cache.get(4) << endl;
    return 0;
}
