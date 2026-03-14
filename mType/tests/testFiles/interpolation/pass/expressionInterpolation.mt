// Test string interpolation with arithmetic expressions
int a = 10;
int b = 20;
string result = $"Result: {a + b}";
print(result);

string calc = $"{a} * {b} = {a * b}";
print(calc);
