// Author: Mashhood Ali
// Day 13 - Bloom Filter (C++)
// A Bloom filter answers "definitely not in the set" or "probably in
// the set" using only a few bits per element. Databases (Cassandra,
// RocksDB), caches and browsers use it to skip expensive disk or
// network lookups: a miss is guaranteed correct, a hit just means
// "check for real". It never produces a false negative, only a small,
// tunable false-positive rate.
//
// Approach: m bits and k hash functions. To add an item, set the k
// bit positions its hashes point to; to test it, check all k bits.
// The k positions come from double hashing: h(i) = (h1 + i*h2) mod m,
// so only two real hashes are needed. h1 is FNV-1a, h2 is SplitMix64
// of h1, which spreads nearby strings apart. Given the expected
// element count n and a target false-positive rate p, the optimal
// size is m = -(n ln p) / (ln 2)^2 bits and k = (m / n) ln 2 hashes.
// The filter also supports union (bitwise OR of two filters with the
// same shape) and serialisation to bytes, so it can be saved to disk
// or sent over the network.
// Time: O(k) add / contains, Space: O(m) bits (~1.44 * log2(1/p)
// bits per element at the optimum, e.g. ~10 bits per element at p=1%).

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

class BloomFilter
{
public:
    // Build a filter sized for `expectedItems` at false-positive rate `targetFpRate`.
    BloomFilter(size_t expectedItems, double targetFpRate)
        : BloomFilter(optimalBitCount(expectedItems, targetFpRate),
                      optimalHashCount(expectedItems, targetFpRate))
    {
    }

    // Build a filter with an explicit bit count and hash count.
    BloomFilter(size_t bitCount, size_t hashCount)
        : bits_((bitCount + 7) / 8, 0), bitCount_(bitCount),
          hashCount_(hashCount), inserted_(0)
    {
        if (bitCount == 0 || hashCount == 0)
            throw invalid_argument("bit count and hash count must be > 0");
    }

    // Move-only: the mutex cannot be copied or moved, so a moved-from
    // filter keeps a fresh mutex and its data moves across.
    BloomFilter(BloomFilter&& other) noexcept
        : bits_(std::move(other.bits_)), bitCount_(other.bitCount_),
          hashCount_(other.hashCount_), inserted_(other.inserted_)
    {
    }
    BloomFilter(const BloomFilter&) = delete;
    BloomFilter& operator=(const BloomFilter&) = delete;
    BloomFilter& operator=(BloomFilter&&) = delete;

    void add(const string& item)
    {
        lock_guard<mutex> lock(mutex_);
        const auto [h1, h2] = hashPair(item);
        for (size_t i = 0; i < hashCount_; ++i)
            setBit((h1 + i * h2) % bitCount_);
        ++inserted_;
    }

    // True means "probably present"; false means "definitely absent".
    bool contains(const string& item) const
    {
        lock_guard<mutex> lock(mutex_);
        const auto [h1, h2] = hashPair(item);
        for (size_t i = 0; i < hashCount_; ++i)
            if (!getBit((h1 + i * h2) % bitCount_))
                return false;
        return true;
    }

    // Union: every item in either filter is reported present by the result.
    // Both filters must have the same bit count and hash count.
    BloomFilter mergedWith(const BloomFilter& other) const
    {
        if (bitCount_ != other.bitCount_ || hashCount_ != other.hashCount_)
            throw invalid_argument("cannot merge filters with different shapes");
        lock_guard<mutex> lock(mutex_);
        lock_guard<mutex> otherLock(other.mutex_);
        BloomFilter result(bitCount_, hashCount_);
        for (size_t i = 0; i < bits_.size(); ++i)
            result.bits_[i] = bits_[i] | other.bits_[i];
        result.inserted_ = inserted_ + other.inserted_;
        return result;
    }

    // Estimated false-positive rate for the items inserted so far.
    double estimatedFalsePositiveRate() const
    {
        lock_guard<mutex> lock(mutex_);
        const double fill = 1.0 - exp(-static_cast<double>(hashCount_ * inserted_) /
                                      static_cast<double>(bitCount_));
        return pow(fill, static_cast<double>(hashCount_));
    }

    vector<uint8_t> serialize() const
    {
        lock_guard<mutex> lock(mutex_);
        vector<uint8_t> out;
        out.reserve(16 + bits_.size());
        appendNumber(out, static_cast<uint64_t>(bitCount_));
        appendNumber(out, static_cast<uint64_t>(hashCount_));
        out.insert(out.end(), bits_.begin(), bits_.end());
        return out;
    }

    static BloomFilter deserialize(const vector<uint8_t>& bytes)
    {
        if (bytes.size() < 16)
            throw invalid_argument("serialised filter is too short");
        const uint64_t bitCount = readNumber(bytes, 0);
        const uint64_t hashCount = readNumber(bytes, 8);
        if (bytes.size() != 16 + (bitCount + 7) / 8)
            throw invalid_argument("serialised filter has the wrong length");
        BloomFilter filter(static_cast<size_t>(bitCount), static_cast<size_t>(hashCount));
        memcpy(filter.bits_.data(), bytes.data() + 16, filter.bits_.size());
        return filter;
    }

    size_t bitCount() const { return bitCount_; }
    size_t hashCount() const { return hashCount_; }
    size_t insertedCount() const { return inserted_; }

    size_t bitsSet() const
    {
        lock_guard<mutex> lock(mutex_);
        size_t count = 0;
        for (uint8_t byte : bits_)
            count += static_cast<size_t>(__builtin_popcount(byte));
        return count;
    }

    static size_t optimalBitCount(size_t n, double p)
    {
        if (n == 0 || p <= 0.0 || p >= 1.0)
            throw invalid_argument("expected items must be > 0 and rate in (0, 1)");
        return static_cast<size_t>(ceil(-(static_cast<double>(n) * log(p)) / (log(2.0) * log(2.0))));
    }

    static size_t optimalHashCount(size_t n, double p)
    {
        const size_t m = optimalBitCount(n, p);
        return max<size_t>(1, static_cast<size_t>(round((static_cast<double>(m) / static_cast<double>(n)) * log(2.0))));
    }

private:
    static pair<uint64_t, uint64_t> hashPair(const string& item)
    {
        // h1: FNV-1a over the bytes. h2: SplitMix64 of h1, so the two
        // hashes are independent even for near-identical strings.
        uint64_t h1 = 14695981039346656037ULL;
        for (unsigned char c : item)
        {
            h1 ^= c;
            h1 *= 1099511628211ULL;
        }
        uint64_t h2 = h1 + 0x9e3779b97f4a7c15ULL;
        h2 = (h2 ^ (h2 >> 30)) * 0xbf58476d1ce4e5b9ULL;
        h2 = (h2 ^ (h2 >> 27)) * 0x94d049bb133111ebULL;
        h2 = h2 ^ (h2 >> 31);
        return {h1, h2 == 0 ? 1 : h2};
    }

    void setBit(size_t index) { bits_[index / 8] |= static_cast<uint8_t>(1u << (index % 8)); }
    bool getBit(size_t index) const { return (bits_[index / 8] >> (index % 8)) & 1u; }

    static void appendNumber(vector<uint8_t>& out, uint64_t value)
    {
        for (int i = 0; i < 8; ++i)
            out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }

    static uint64_t readNumber(const vector<uint8_t>& bytes, size_t offset)
    {
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i)
            value |= static_cast<uint64_t>(bytes[offset + i]) << (8 * i);
        return value;
    }

    vector<uint8_t> bits_;
    size_t bitCount_;
    size_t hashCount_;
    size_t inserted_ = 0;
    mutable mutex mutex_;
};

static int failures = 0;
static void check(bool ok, const string& label)
{
    cout << label << ": " << (ok ? "OK" : "FAILED") << '\n';
    if (!ok) ++failures;
}

int main()
{
    const size_t n = 10000;
    BloomFilter filter(n, 0.01);
    cout << "filter for " << n << " items at 1% fp rate: "
         << filter.bitCount() << " bits (" << filter.bitCount() / 8
         << " bytes), " << filter.hashCount() << " hashes\n";

    for (size_t i = 0; i < n; ++i)
        filter.add("user:" + to_string(i));

    size_t missed = 0;
    for (size_t i = 0; i < n; ++i)
        if (!filter.contains("user:" + to_string(i)))
            ++missed;
    check(missed == 0, "no false negatives on 10000 inserted keys");

    size_t falsePositives = 0;
    for (size_t i = 0; i < n; ++i)
        if (filter.contains("other:" + to_string(i)))
            ++falsePositives;
    cout << "false positives on 10000 absent keys: " << falsePositives
         << " (" << (100.0 * falsePositives / n) << "%)\n";
    check(falsePositives < 300, "false-positive rate under 3%");

    cout << "estimated false-positive rate: "
         << filter.estimatedFalsePositiveRate() * 100.0 << "%\n";
    check(filter.estimatedFalsePositiveRate() < 0.03, "estimated rate under 3%");

    check(!filter.contains("definitely-not-there"), "absent key reported absent");
    check(filter.bitsSet() > 0 && filter.bitsSet() < filter.bitCount(),
          "some bits set, not all");

    BloomFilter restored = BloomFilter::deserialize(filter.serialize());
    check(restored.contains("user:42") && !restored.contains("other:42"),
          "serialise/deserialise round-trip keeps membership");

    BloomFilter left(1000, 0.01), right(1000, 0.01);
    left.add("alpha");
    right.add("beta");
    BloomFilter both = left.mergedWith(right);
    check(both.contains("alpha") && both.contains("beta"), "union contains both sets");

    bool badMerge = false;
    try
    {
        BloomFilter wide(2000, 0.01);
        (void)left.mergedWith(wide);
    }
    catch (const invalid_argument&)
    {
        badMerge = true;
    }
    check(badMerge, "merging mismatched filters throws");

    if (failures == 0)
        cout << "All checks passed.\n";
    else
        cout << failures << " check(s) FAILED.\n";
    return failures == 0 ? 0 : 1;
}
