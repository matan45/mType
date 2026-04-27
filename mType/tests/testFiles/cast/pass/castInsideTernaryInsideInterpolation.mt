// Test: ternary expression inside string interpolation, with a cast inside the ternary
bool b = true;
int x = 5;
int y = 10;

// Plain ternary inside interpolation
print($"value: {(b ? x : y)}");

// Cast inside the ternary inside the interpolation
float f = 3.7;
print($"casted: {(b ? (int)f : y)}");

// Negated branch
b = false;
print($"value: {(b ? x : y)}");
