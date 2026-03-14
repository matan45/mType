// Test that STRING_BUILD opcode handles many segments correctly
// This exercises the O(n) path instead of O(n^2) chained ADD
string a = "A";
string b = "B";
string c = "C";
string d = "D";
string e = "E";

// 3+ segments triggers STRING_BUILD
string r1 = $"{a}-{b}-{c}";
print(r1);

// 5 expressions + 4 text segments = 9 segments
string r2 = $"{a},{b},{c},{d},{e}";
print(r2);

// Mixed types with many segments
int x = 1;
float y = 2.5;
bool z = true;
string r3 = $"x={x} y={y} z={z} a={a}";
print(r3);

// Large interpolation chain
string r4 = $"{a}{b}{c}{d}{e}";
print(r4);
