// Test: interpolation with a nested string literal expression inside the braces
string name = "world";
string result = $"say {"hello"} to {name}";
print(result);
