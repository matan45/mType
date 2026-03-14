// Test interpolation with whitespace-only text segments
string x = "A";
string y = "B";

// Whitespace between interpolations
string r1 = $"{x} {y}";
print(r1);

// Tab between interpolations
string r2 = $"{x}\t{y}";
print(r2);

// Newline in interpolation text
string r3 = $"{x}\n{y}";
print(r3);
