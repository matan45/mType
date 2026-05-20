// Error: substring start index past end-of-string throws
string s = "short";
string sub = substring(s, 100, 5);
print("should not reach: " + sub);
