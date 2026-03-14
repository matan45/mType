// Test interpolation where the expression itself involves a string literal
string result = $"greeting: {"hello"}";
print(result);

// Ternary with string literals inside interpolation
bool flag = true;
string r2 = $"value: {flag ? "yes" : "no"}";
print(r2);
