// MYT-220 regression guard: the new (T?) cast lookahead must not
// misclassify a ternary inside parens as a cast.

int x = 5;
int y = (x > 0 ? 1 : 2);
print(y);

// Expected output:
// 1
