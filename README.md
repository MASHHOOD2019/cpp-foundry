# cpp-foundry

One small C++ program every day. No filler commits, no empty files.

## Day 1 - Grade Calculator (C++)

`day-01-grade-calculator/grade_calculator.cpp`

Enter your courses with credits and marks, it prints each course with its
grade points and your semester GPA.

How to run:

```
g++ grade_calculator.cpp -o grade_calculator
./grade_calculator
```

Example:

```
How many courses? 3
Course 1 name: OOP
Credits: 3
Marks (0-100): 82
OOP (3 credits) - Marks: 82, Points: 3.33
...
Semester GPA: 2.88667
```

## Day 3 - Two Sum (C++)

`day-03-two-sum/two_sum.cpp`

Given an array of numbers and a target, find the two numbers that add up
to the target and return their positions. Classic interview problem.

Solved in one pass with a hash map: for each number, check if
(target - number) was already seen. Time O(n), space O(n).
The brute-force double loop is O(n^2).

How to run:

```
g++ two_sum.cpp -o two_sum
./two_sum
```

Example:

```
Positions: 0 and 1
Numbers: 2 + 7 = 9
```

## Day 5 - LRU Cache (C++)

`day-05-lru-cache/lru_cache.cpp`

A fixed-capacity cache that evicts the least recently used item.
Classic interview and systems problem.

Solved with a doubly linked list for use order plus a hash map
pointing at each node, so get and put both run in O(1) time.
Space O(capacity).

How to run:

```
g++ lru_cache.cpp -o lru_cache
./lru_cache
```

Example:

```
get(1): 1
get(2): -1
get(1): -1
get(3): 3
get(4): 4
```

## Day 7 - Token Bucket Rate Limiter (C++)

`day-07-rate-limiter/rate_limiter.cpp`

Limits how many requests a caller may make in a time window.
The bucket holds tokens, refills at a fixed rate, and each
request spends one token. Requests with an empty bucket are
rejected instead of queued, so a burst can never overwhelm
the service behind it. This is the limiter behind most API
gateways.

Solved with lazy refill from the elapsed wall-clock time, so
there is no background thread and no timer. A mutex guards
the token count, so the limiter is safe to share across
threads. Time O(1) per request, space O(1).

How to run:

```
g++ -std=c++17 -pthread rate_limiter.cpp -o rate_limiter
./rate_limiter
```

Example:

```
Burst of 8 requests (capacity 5, refill 2/s):
  request 1: allowed
  ...
  request 6: rejected
After waiting 1s, tokens available: 2
  request 9: allowed
  request 10: allowed
  request 11: rejected
40 concurrent requests on a 10-token bucket: 10 allowed
```

## Day 9 - Thread Pool (C++)

`day-09-thread-pool/thread_pool.cpp`

A fixed pool of worker threads that runs submitted tasks in
parallel. Instead of spawning a thread per job, the pool
reuses a small set of threads and hands them work from a
shared queue. This is the pattern behind every web server,
game engine and job queue.

Workers wait on a condition variable and wake when a task
arrives. submit() wraps any callable in a packaged_task and
returns a std::future, so the caller collects the result (or
the exception) later. The destructor stops the queue, drains
the remaining tasks and joins every worker, so no task is
dropped and no thread is left running. Time O(1) amortized
per submit, space O(queued tasks).

How to run:

```
g++ -std=c++17 -pthread thread_pool.cpp -o thread_pool
./thread_pool
```

Example:

```
Parallel sum 1..1000000: 500000500000 (expected 500000500000) -> OK
Sum of squares 0..99: 328350 (expected 328350) -> OK
Exception test: caught "task failed on purpose" -> OK
Shutdown drain: 5 queued tasks completed, sum 15 (expected 15) -> OK
All checks passed.
```
