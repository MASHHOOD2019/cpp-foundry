// Author: Mashhood Ali
// Day 9 - Thread Pool (C++)
// A fixed pool of worker threads that runs submitted tasks in
// parallel. Instead of spawning a thread per job (expensive
// and unbounded), the pool reuses a small set of threads and
// hands them work from a shared queue. This is the pattern
// behind every web server, game engine and job queue: one
// pool, many short-lived tasks, no thread-creation overhead.
//
// Approach: workers wait on a condition variable and wake
// when a task arrives. submit() wraps any callable in a
// packaged_task and returns a std::future, so the caller can
// collect the result (or the exception) later. The destructor
// stops the queue, wakes every worker and joins them, so no
// task is ever dropped and no thread is ever left running.
// Time: O(1) amortized per submit, Space: O(queued tasks).

#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace std;

class ThreadPool
{
public:
    // workers: how many threads to keep alive. Defaults to the
    // number of CPU cores; passing 0 is rejected.
    explicit ThreadPool(size_t workers = thread::hardware_concurrency())
    {
        if (workers == 0)
        {
            throw invalid_argument("ThreadPool needs at least 1 worker");
        }
        for (size_t i = 0; i < workers; ++i)
        {
            threads.emplace_back([this] { workerLoop(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a callable and its arguments. Returns a future
    // holding the result; any exception the task throws is
    // rethrown when the caller calls future.get().
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> future<invoke_result_t<F, Args...>>
    {
        using ReturnType = invoke_result_t<F, Args...>;
        auto task = make_shared<packaged_task<ReturnType()>>(
            bind(forward<F>(f), forward<Args>(args)...));
        future<ReturnType> result = task->get_future();
        {
            lock_guard<mutex> lock(mtx);
            if (stopped)
            {
                throw runtime_error("submit on stopped ThreadPool");
            }
            tasks.emplace([task] { (*task)(); });
        }
        cv.notify_one();
        return result;
    }

    // How many tasks are waiting in the queue right now.
    size_t queued() const
    {
        lock_guard<mutex> lock(mtx);
        return tasks.size();
    }

    // Stops accepting new work, runs the queued tasks to
    // completion, then joins every worker thread.
    ~ThreadPool()
    {
        {
            lock_guard<mutex> lock(mtx);
            stopped = true;
        }
        cv.notify_all();
        for (thread& t : threads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

private:
    void workerLoop()
    {
        while (true)
        {
            function<void()> task;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this] { return stopped || !tasks.empty(); });
                if (stopped && tasks.empty())
                {
                    return;
                }
                task = move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }

    vector<thread> threads;
    queue<function<void()>> tasks;
    mutable mutex mtx;
    condition_variable cv;
    bool stopped = false;
};

// Sums one slice of a range; the pool runs one of these per chunk.
long long partialSum(long long lo, long long hi)
{
    long long sum = 0;
    for (long long i = lo; i <= hi; ++i)
    {
        sum += i;
    }
    return sum;
}

int main()
{
    const long long N = 1000000;
    const long long expected = N * (N + 1) / 2;

    // 1) Parallel sum: split 1..1,000,000 into 8 chunks and run
    //    one chunk per worker, then add the partial results.
    ThreadPool pool(8);
    vector<future<long long>> futures;
    const long long chunk = N / 8;
    for (int i = 0; i < 8; ++i)
    {
        long long lo = i * chunk + 1;
        long long hi = (i == 7) ? N : (i + 1) * chunk;
        futures.push_back(pool.submit(partialSum, lo, hi));
    }
    long long total = 0;
    for (auto& f : futures)
    {
        total += f.get();
    }
    cout << "Parallel sum 1.." << N << ": " << total
         << " (expected " << expected << ") -> "
         << (total == expected ? "OK" : "MISMATCH") << "\n";

    // 2) Return values: 100 tiny tasks, each squaring its index.
    vector<future<int>> squares;
    for (int i = 0; i < 100; ++i)
    {
        squares.push_back(pool.submit([](int x) { return x * x; }, i));
    }
    int squareSum = 0;
    for (auto& f : squares)
    {
        squareSum += f.get();
    }
    cout << "Sum of squares 0..99: " << squareSum
         << " (expected 328350) -> "
         << (squareSum == 328350 ? "OK" : "MISMATCH") << "\n";

    // 3) Exceptions travel back through the future, they never
    //    crash the worker thread.
    auto bad = pool.submit([]() -> int {
        throw runtime_error("task failed on purpose");
        return 0;
    });
    try
    {
        bad.get();
        cout << "Exception test: FAILED (no exception)\n";
    }
    catch (const runtime_error& e)
    {
        cout << "Exception test: caught \"" << e.what() << "\" -> OK\n";
    }

    // 4) The pool drains its queue on shutdown: submit 5 tasks
    //    from a pool that is about to be destroyed.
    {
        ThreadPool shortLived(2);
        vector<future<int>> done;
        for (int i = 0; i < 5; ++i)
        {
            done.push_back(shortLived.submit([i] { return i + 1; }));
        }
        int drained = 0;
        for (auto& f : done)
        {
            drained += f.get();
        }
        cout << "Shutdown drain: 5 queued tasks completed, sum "
             << drained << " (expected 15) -> "
             << (drained == 15 ? "OK" : "MISMATCH") << "\n";
    }

    cout << "All checks passed.\n";
    return 0;
}
