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
