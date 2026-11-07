// Author: Mashhood Ali
// Day 3 - Two Sum (C++)
// Given an array of numbers and a target, find two numbers
// that add up to the target and return their positions.
// Example: nums = [2, 7, 11, 15], target = 9 -> [0, 1]
// because 2 + 7 = 9.

#include <iostream>
#include <vector>
using namespace std;

vector<int> twoSum(vector<int> nums, int target)
{
    vector<int> answer;
    for (int i = 0; i < nums.size(); i++)
    {
        for (int j = i + 1; j < nums.size(); j++)
        {
            if (nums[i] + nums[j] == target)
            {
                answer.push_back(i);
                answer.push_back(j);
                return answer;
            }
        }
    }
    return answer;
}

int main()
{
    vector<int> nums;
    nums.push_back(2);
    nums.push_back(7);
    nums.push_back(11);
    nums.push_back(15);
    int target = 9;

    vector<int> result = twoSum(nums, target);

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
