// Test single statement blocks with minimal syntax

// Single statement in if without braces (if mType supports it, otherwise with braces)
int x = 10;
if (x > 5) {
    print(1);
}

if (x < 20) {
    print(2);
}

// Single statement in else
if (x < 5) {
    print(0);
} else {
    print(3);
}

// Single statement in while loop
int counter = 0;
while (counter < 3) {
    counter = counter + 1;
}
print(counter);

// Single statement for loop equivalents (using while)
int i = 0;
while (i < 5) {
    i = i + 1;
}
print(i);

// Nested single statement blocks
if (x > 0) {
    if (x > 5) {
        print(4);
    }
}

// Single statement with function call
if (true) {
    print(5);
}

// Single return statement in function
function singleReturn(int val): int {
    return val * 2;
}

print(singleReturn(3));

// Single statement in else-if chain
int val = 2;
if (val == 1) {
    print(0);
} else if (val == 2) {
    print(6);
} else {
    print(0);
}

// Combination of single and multi-statement blocks
if (x > 5) {
    print(7);
} else {
    print(0);
    print(0);
}

// Single statement with complex expression
if (x * 2 - 5 > 10) {
    print(8);
}

// Single statement in nested conditions
if (x > 0) {
    if (x < 100) {
        print(9);
    }
}

// Single statement assignments
int a = 0;
if (true) {
    a = 100;
}
print(a);

// Single break in loop
int j = 0;
while (j < 10) {
    j = j + 1;
    if (j == 5) {
        break;
    }
}
print(j);

// Single continue in loop
int k = 0;
int sum = 0;
while (k < 5) {
    k = k + 1;
    if (k == 3) {
        continue;
    }
    sum = sum + k;
}
print(sum);

print("Test passed"); // Test completed
