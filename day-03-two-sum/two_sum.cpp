// Author: Mashhood Ali
// Day 3 - Two Sum (C++)
// Given an array of numbers and a target, find two numbers
// that add up to the target and return their positions.
// Example: nums = [2, 7, 11, 15], target = 9 -> [0, 1]
// because 2 + 7 = 9.
//
// Approach: one pass with a hash map. For each number, check if
// (target - number) was already seen. If yes, we have the pair.
// Time: O(n), Space: O(n). The brute-force double loop is O(n^2).

#include <iostream>
#include <vector>
#include <unordered_map>
using namespace std;

class Solution
{
public:
    vector<int> twoSum(vector<int> &nums, int target)
    {
        unordered_map<int, int> seen;
        int n = nums.size();
        for (int i = 0; i < n; i++)
        {
            int needed = target - nums[i];
            if (seen.find(needed) != seen.end())
            {
                return {seen[needed], i};
            }
            seen[nums[i]] = i;
        }
        return {};
    }
};

int main()
{
    Solution sol;
    vector<int> nums = {2, 7, 11, 15};
    int target = 9;

    vector<int> result = sol.twoSum(nums, target);

    if (result.size() == 2)
    {
        cout << "Positions: " << result[0] << " and " << result[1] << endl;
        cout << "Numbers: " << nums[result[0]] << " + " << nums[result[1]] << " = " << target << endl;
    }
    else
    {
        cout << "No two numbers add up to the target." << endl;
    }

    return 0;
}
