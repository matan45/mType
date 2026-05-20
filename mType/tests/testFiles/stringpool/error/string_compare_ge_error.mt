// Error: relational >= on strings is rejected (MT-E5005)
string a = "alpha";
string b = "beta";
bool r = a >= b;
print("should not reach: " + r);
