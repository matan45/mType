// Error: substring with negative start index throws
string s = "hello";
string sub = substring(s, -1, 5);
print("should not reach: " + sub);
