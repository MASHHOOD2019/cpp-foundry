// Author: Mashhood Ali
// Day 1 - Grade Calculator
// Takes courses with credits and marks, prints semester GPA.

#include <iostream>
#include <string>
using namespace std;

class Course {
private:
    string name;
    int credits;
    int marks;

public:
    Course(string n, int c, int m) {
        name = n;
        credits = c;
        marks = m;
    }

    string getName() {
        return name;
    }

    int getCredits() {
        return credits;
    }

    // marks to grade points, simple 4.0 scale
    double getPoints() {
        if (marks >= 90)
            return 4.0;
        else if (marks >= 85)
            return 3.67;
        else if (marks >= 80)
            return 3.33;
        else if (marks >= 75)
            return 3.0;
        else if (marks >= 70)
            return 2.67;
        else if (marks >= 65)
            return 2.33;
        else if (marks >= 60)
            return 2.0;
        else if (marks >= 55)
            return 1.67;
        else if (marks >= 50)
            return 1.0;
        else
            return 0.0;
    }

    void show() {
        cout << name << " (" << credits << " credits) - Marks: " << marks
             << ", Points: " << getPoints() << endl;
    }
};

int main() {
    int n;
    cout << "How many courses? ";
    cin >> n;

    double totalPoints = 0;
    int totalCredits = 0;
    int temp = 0; // used below for total credits check

    for (int i = 0; i < n; i++) {
        string name;
        int credits, marks;
        cout << "Course " << i + 1 << " name: ";
        cin >> name;
        cout << "Credits: ";
        cin >> credits;
        cout << "Marks (0-100): ";
        cin >> marks;

        Course c(name, credits, marks);
        c.show();
        totalPoints = totalPoints + c.getPoints() * credits;
        totalCredits = totalCredits + credits;
    }

    temp = totalCredits;
    if (temp > 0) {
        double gpa = totalPoints / temp;
        cout << "Semester GPA: " << gpa << endl;
    }
    else {
        cout << "No credits entered." << endl;
    }

    return 0;
}
