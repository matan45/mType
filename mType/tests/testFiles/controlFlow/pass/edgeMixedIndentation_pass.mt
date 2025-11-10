// Test various indentation styles (should all work correctly)

// No indentation in function
function noIndent(): int {
return 42;
}

print(noIndent());

// Mixed indentation in if-else
int x = 10;
if (x > 5) {
print(1);
} else {
    print(0);
}

// Varying indentation levels
if (x > 0) {
  if (x > 5) {
    if (x > 8) {
        print(2);
    }
  }
}

// No indentation in loops
int i = 0;
while (i < 3) {
i = i + 1;
}
print(i);

// Function with inconsistent internal indentation
function mixedIndent(int a, int b): int {
int result = 0;
  if (a > b) {
result = a;
  } else {
    result = b;
  }
    return result;
}

print(mixedIndent(10, 20));

// Nested blocks with varying indentation
{
    int a = 1;
  {
        int b = 2;
    {
      int c = 3;
        print(a + b + c);
    }
  }
}

// While loop with mixed indentation
int counter = 0;
int sum = 0;
while (counter < 5) {
counter = counter + 1;
  sum = sum + counter;
}
print(sum);

// If-else chain with varying indentation
int val = 25;
if (val < 10) {
print(0);
} else if (val < 20) {
    print(0);
} else if (val < 30) {
  print(3);
} else {
        print(0);
}

// Function call indentation variations
print(1);
  print(2);
    print(3);
print(4);

// Nested conditionals with no indentation
if (true) {
if (true) {
if (true) {
print(5);
}
}
}

// Block with single line multiple statements possible indentation
int p = 0;
int q = 0;
if (true) {
p = 10;
q = 20;
}
print(p + q);

// Expression with varied spacing (still valid)
int result = 1+2*3-4/2;
print(result);

print("Test passed"); // Test completed
