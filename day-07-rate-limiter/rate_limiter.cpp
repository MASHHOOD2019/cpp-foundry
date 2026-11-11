// Author: Mashhood Ali
// Day 7 - Token Bucket Rate Limiter (C++)
// Limits how many requests a caller may make in a time window.
// The bucket holds tokens, refills at a fixed rate, and each
// request spends one token. Requests with an empty bucket are
// rejected instead of queued, so a burst can never overwhelm
// the service behind it. This is the limiter behind most API
// gateways (for example GitHub allows 5000 requests per hour).
//
// Approach: tokens refill lazily on every call from the elapsed
// wall-clock time, so there is no background thread and no timer.
// A mutex guards the token count and timestamp, so the limiter
// is safe to share across threads.
// Time: O(1) per request, Space: O(1).

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
using namespace std;
using namespace std::chrono;

class RateLimiter
{
public:
    // capacity: max burst size (tokens the bucket can hold).
    // refillRate: tokens added per second.
    RateLimiter(double capacity, double refillRate)
        : capacity(capacity), refillRate(refillRate),
          tokens(capacity), lastRefill(steady_clock::now()) {}

    // Returns true when the request may proceed, false when
    // the caller must back off and retry later.
    bool allow()
    {
        lock_guard<mutex> lock(mtx);
        refill();
        if (tokens < 1.0)
        {
            return false;
        }
        tokens -= 1.0;
        return true;
    }

    double availableTokens()
    {
        lock_guard<mutex> lock(mtx);
        refill();
        return tokens;
    }

private:
    void refill()
    {
        auto now = steady_clock::now();
        double elapsed = duration<double>(now - lastRefill).count();
        tokens = min(capacity, tokens + elapsed * refillRate);
        lastRefill = now;
    }

    double capacity;
    double refillRate;
    double tokens;
    steady_clock::time_point lastRefill;
    mutex mtx;
};

int main()
{
    // Bucket of 5 tokens, refilling 2 per second.
    RateLimiter limiter(5, 2);

    cout << "Burst of 8 requests (capacity 5, refill 2/s):" << endl;
    for (int i = 1; i <= 8; i++)
    {
        cout << "  request " << i << ": "
             << (limiter.allow() ? "allowed" : "rejected") << endl;
    }

    // After 1 second, 2 tokens have refilled.
    this_thread::sleep_for(seconds(1));
    cout << "After waiting 1s, tokens available: "
         << limiter.availableTokens() << endl;
    cout << "  request 9: "
         << (limiter.allow() ? "allowed" : "rejected") << endl;
    cout << "  request 10: "
         << (limiter.allow() ? "allowed" : "rejected") << endl;
    cout << "  request 11: "
         << (limiter.allow() ? "allowed" : "rejected") << endl;

    // Four threads hammering one shared limiter: at most the
    // burst size can ever get through at once. Refill is 0 here
    // so the count is exact.
    RateLimiter shared(10, 0);
    int allowed = 0;
    mutex countMtx;
    vector<thread> workers;
    for (int t = 0; t < 4; t++)
    {
        workers.emplace_back([&]() {
            for (int i = 0; i < 10; i++)
            {
                if (shared.allow())
                {
                    lock_guard<mutex> lock(countMtx);
                    allowed++;
                }
            }
        });
    }
    for (auto &w : workers)
    {
        w.join();
    }
    cout << "40 concurrent requests on a 10-token bucket: "
         << allowed << " allowed" << endl;

    return 0;
}
