// Modern chained ternary operator patterns
// Tests complex ternary expressions for concise conditional logic

// Test 1: Simple ternary chains for categorization
print("Test 1: Ternary chains for number categorization");
int num1 = -5;
string cat1 = num1 < 0 ? "negative" : (num1 == 0 ? "zero" : (num1 < 10 ? "small" : "large"));
print(cat1);

int num2 = 0;
string cat2 = num2 < 0 ? "negative" : (num2 == 0 ? "zero" : (num2 < 10 ? "small" : "large"));
print(cat2);

int num3 = 5;
string cat3 = num3 < 0 ? "negative" : (num3 == 0 ? "zero" : (num3 < 10 ? "small" : "large"));
print(cat3);

int num4 = 15;
string cat4 = num4 < 0 ? "negative" : (num4 == 0 ? "zero" : (num4 < 10 ? "small" : "large"));
print(cat4);

// Test 2: Ternary chains for grade assignment
print("Test 2: Ternary chains for grades");
int score1 = 95;
string grade1 = score1 >= 90 ? "A" : (score1 >= 80 ? "B" : (score1 >= 70 ? "C" : (score1 >= 60 ? "D" : "F")));
print(grade1);

int score2 = 85;
string grade2 = score2 >= 90 ? "A" : (score2 >= 80 ? "B" : (score2 >= 70 ? "C" : (score2 >= 60 ? "D" : "F")));
print(grade2);

int score3 = 75;
string grade3 = score3 >= 90 ? "A" : (score3 >= 80 ? "B" : (score3 >= 70 ? "C" : (score3 >= 60 ? "D" : "F")));
print(grade3);

int score4 = 65;
string grade4 = score4 >= 90 ? "A" : (score4 >= 80 ? "B" : (score4 >= 70 ? "C" : (score4 >= 60 ? "D" : "F")));
print(grade4);

int score5 = 55;
string grade5 = score5 >= 90 ? "A" : (score5 >= 80 ? "B" : (score5 >= 70 ? "C" : (score5 >= 60 ? "D" : "F")));
print(grade5);

// Test 3: Ternary chains with numeric results
print("Test 3: Ternary chains for numeric calculations");
int value1 = 5;
int result1 = value1 < 0 ? -1 : (value1 == 0 ? 0 : (value1 < 10 ? value1 * 2 : value1 * 3));
print(result1);

int value2 = 0;
int result2 = value2 < 0 ? -1 : (value2 == 0 ? 0 : (value2 < 10 ? value2 * 2 : value2 * 3));
print(result2);

int value3 = 15;
int result3 = value3 < 0 ? -1 : (value3 == 0 ? 0 : (value3 < 10 ? value3 * 2 : value3 * 3));
print(result3);

// Test 4: Nested ternary with boolean conditions
print("Test 4: Nested ternary with boolean logic");
bool flag1 = true;
bool flag2 = false;
string logicResult1 = flag1 ? (flag2 ? "both-true" : "first-true") : (flag2 ? "second-true" : "both-false");
print(logicResult1);

flag1 = false;
flag2 = true;
string logicResult2 = flag1 ? (flag2 ? "both-true" : "first-true") : (flag2 ? "second-true" : "both-false");
print(logicResult2);

flag1 = false;
flag2 = false;
string logicResult3 = flag1 ? (flag2 ? "both-true" : "first-true") : (flag2 ? "second-true" : "both-false");
print(logicResult3);

flag1 = true;
flag2 = true;
string logicResult4 = flag1 ? (flag2 ? "both-true" : "first-true") : (flag2 ? "second-true" : "both-false");
print(logicResult4);

// Test 5: Ternary chains for tier classification
print("Test 5: Ternary chains for tier systems");
int points1 = 150;
string tier1 = points1 >= 100 ? "gold" : (points1 >= 50 ? "silver" : (points1 >= 10 ? "bronze" : "basic"));
print(tier1);

int points2 = 75;
string tier2 = points2 >= 100 ? "gold" : (points2 >= 50 ? "silver" : (points2 >= 10 ? "bronze" : "basic"));
print(tier2);

int points3 = 25;
string tier3 = points3 >= 100 ? "gold" : (points3 >= 50 ? "silver" : (points3 >= 10 ? "bronze" : "basic"));
print(tier3);

int points4 = 5;
string tier4 = points4 >= 100 ? "gold" : (points4 >= 50 ? "silver" : (points4 >= 10 ? "bronze" : "basic"));
print(tier4);

// Test 6: Ternary chains with mathematical expressions
print("Test 6: Ternary chains with math expressions");
int x1 = 10;
int y1 = 5;
int mathResult1 = x1 > y1 ? (x1 > y1 * 2 ? x1 * 2 : x1 + y1) : (y1 > x1 * 2 ? y1 * 2 : y1 + x1);
print(mathResult1);

int x2 = 3;
int y2 = 10;
int mathResult2 = x2 > y2 ? (x2 > y2 * 2 ? x2 * 2 : x2 + y2) : (y2 > x2 * 2 ? y2 * 2 : y2 + x2);
print(mathResult2);

// Test 7: Ternary chains for priority selection
print("Test 7: Ternary chains for priority");
int priority1 = 1;
string action1 = priority1 == 1 ? "critical" : (priority1 == 2 ? "high" : (priority1 == 3 ? "medium" : "low"));
print(action1);

int priority2 = 2;
string action2 = priority2 == 1 ? "critical" : (priority2 == 2 ? "high" : (priority2 == 3 ? "medium" : "low"));
print(action2);

int priority3 = 3;
string action3 = priority3 == 1 ? "critical" : (priority3 == 2 ? "high" : (priority3 == 3 ? "medium" : "low"));
print(action3);

int priority4 = 5;
string action4 = priority4 == 1 ? "critical" : (priority4 == 2 ? "high" : (priority4 == 3 ? "medium" : "low"));
print(action4);

// Test 8: Deeply nested ternary chains
print("Test 8: Deep ternary nesting");
int depth = 3;
string depthResult = depth == 1 ? "one" :
                     (depth == 2 ? "two" :
                      (depth == 3 ? "three" :
                       (depth == 4 ? "four" :
                        (depth == 5 ? "five" : "other"))));
print(depthResult);

depth = 5;
depthResult = depth == 1 ? "one" :
              (depth == 2 ? "two" :
               (depth == 3 ? "three" :
                (depth == 4 ? "four" :
                 (depth == 5 ? "five" : "other"))));
print(depthResult);

depth = 10;
depthResult = depth == 1 ? "one" :
              (depth == 2 ? "two" :
               (depth == 3 ? "three" :
                (depth == 4 ? "four" :
                 (depth == 5 ? "five" : "other"))));
print(depthResult);

// Test 9: Ternary chains with comparison chains
print("Test 9: Ternary chains with comparisons");
int a = 10;
int b = 20;
int c = 15;
int max = a > b ? (a > c ? a : c) : (b > c ? b : c);
print(max);

a = 25;
b = 20;
c = 15;
max = a > b ? (a > c ? a : c) : (b > c ? b : c);
print(max);

a = 10;
b = 20;
c = 30;
max = a > b ? (a > c ? a : c) : (b > c ? b : c);
print(max);

// Test 10: Ternary chains for status determination
class Status {
    public int code;

    constructor(int c) {
        code = c;
    }
}

print("Test 10: Ternary chains with objects");
Status s1 = new Status(200);
string statusMsg1 = s1.code == 200 ? "OK" :
                    (s1.code == 404 ? "Not Found" :
                     (s1.code == 500 ? "Server Error" :
                      (s1.code == 403 ? "Forbidden" : "Unknown")));
print(statusMsg1);

Status s2 = new Status(404);
string statusMsg2 = s2.code == 200 ? "OK" :
                    (s2.code == 404 ? "Not Found" :
                     (s2.code == 500 ? "Server Error" :
                      (s2.code == 403 ? "Forbidden" : "Unknown")));
print(statusMsg2);

Status s3 = new Status(500);
string statusMsg3 = s3.code == 200 ? "OK" :
                    (s3.code == 404 ? "Not Found" :
                     (s3.code == 500 ? "Server Error" :
                      (s3.code == 403 ? "Forbidden" : "Unknown")));
print(statusMsg3);

Status s4 = new Status(403);
string statusMsg4 = s4.code == 200 ? "OK" :
                    (s4.code == 404 ? "Not Found" :
                     (s4.code == 500 ? "Server Error" :
                      (s4.code == 403 ? "Forbidden" : "Unknown")));
print(statusMsg4);

Status s5 = new Status(999);
string statusMsg5 = s5.code == 200 ? "OK" :
                    (s5.code == 404 ? "Not Found" :
                     (s5.code == 500 ? "Server Error" :
                      (s5.code == 403 ? "Forbidden" : "Unknown")));
print(statusMsg5);

// Test 11: Ternary chains for array indexing decisions
print("Test 11: Ternary chains with array operations");
int[] arr = new int[3];
arr[0] = 100;
arr[1] = 200;
arr[2] = 300;

int index1 = 0;
int selectedValue1 = index1 == 0 ? arr[0] : (index1 == 1 ? arr[1] : arr[2]);
print(selectedValue1);

int index2 = 1;
int selectedValue2 = index2 == 0 ? arr[0] : (index2 == 1 ? arr[1] : arr[2]);
print(selectedValue2);

int index3 = 2;
int selectedValue3 = index3 == 0 ? arr[0] : (index3 == 1 ? arr[1] : arr[2]);
print(selectedValue3);

// Test 12: Mixed ternary chains with function calls
function getValue(int multiplier): int {
    return multiplier * 10;
}

print("Test 12: Ternary chains with function calls");
int input1 = 5;
int funcResult1 = input1 < 3 ? getValue(1) : (input1 < 7 ? getValue(2) : getValue(3));
print(funcResult1);

int input2 = 2;
int funcResult2 = input2 < 3 ? getValue(1) : (input2 < 7 ? getValue(2) : getValue(3));
print(funcResult2);

int input3 = 8;
int funcResult3 = input3 < 3 ? getValue(1) : (input3 < 7 ? getValue(2) : getValue(3));
print(funcResult3);

// Test 13: Ternary chains for temperature classification
print("Test 13: Ternary chains for temperature");
int temp1 = -5;
string tempDesc1 = temp1 < 0 ? "freezing" : (temp1 < 15 ? "cold" : (temp1 < 25 ? "mild" : (temp1 < 35 ? "warm" : "hot")));
print(tempDesc1);

int temp2 = 10;
string tempDesc2 = temp2 < 0 ? "freezing" : (temp2 < 15 ? "cold" : (temp2 < 25 ? "mild" : (temp2 < 35 ? "warm" : "hot")));
print(tempDesc2);

int temp3 = 20;
string tempDesc3 = temp3 < 0 ? "freezing" : (temp3 < 15 ? "cold" : (temp3 < 25 ? "mild" : (temp3 < 35 ? "warm" : "hot")));
print(tempDesc3);

int temp4 = 30;
string tempDesc4 = temp4 < 0 ? "freezing" : (temp4 < 15 ? "cold" : (temp4 < 25 ? "mild" : (temp4 < 35 ? "warm" : "hot")));
print(tempDesc4);

int temp5 = 40;
string tempDesc5 = temp5 < 0 ? "freezing" : (temp5 < 15 ? "cold" : (temp5 < 25 ? "mild" : (temp5 < 35 ? "warm" : "hot")));
print(tempDesc5);

print("Test passed");
