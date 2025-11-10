// Test maximum nesting depth for various control structures

// Deeply nested if statements (10 levels)
int value = 100;
if (value > 0) {
    if (value > 10) {
        if (value > 20) {
            if (value > 30) {
                if (value > 40) {
                    if (value > 50) {
                        if (value > 60) {
                            if (value > 70) {
                                if (value > 80) {
                                    if (value > 90) {
                                        print(1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Deeply nested loops (5 levels)
int a = 0;
int b = 0;
int c = 0;
int d = 0;
int e = 0;
int counter = 0;

while (a < 2) {
    while (b < 2) {
        while (c < 2) {
            while (d < 2) {
                while (e < 2) {
                    counter = counter + 1;
                    e = e + 1;
                }
                e = 0;
                d = d + 1;
            }
            d = 0;
            c = c + 1;
        }
        c = 0;
        b = b + 1;
    }
    b = 0;
    a = a + 1;
}
print(counter);

// Deeply nested blocks
{
    int x1 = 1;
    {
        int x2 = 2;
        {
            int x3 = 3;
            {
                int x4 = 4;
                {
                    int x5 = 5;
                    {
                        int x6 = 6;
                        {
                            int x7 = 7;
                            {
                                int x8 = 8;
                                print(x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Deeply nested conditionals with else-if
int score = 95;
if (score < 10) {
    print(0);
} else if (score < 20) {
    print(0);
} else if (score < 30) {
    print(0);
} else if (score < 40) {
    print(0);
} else if (score < 50) {
    print(0);
} else if (score < 60) {
    print(0);
} else if (score < 70) {
    print(0);
} else if (score < 80) {
    print(0);
} else if (score < 90) {
    print(0);
} else {
    print(4);
}

// Nested ternary operators (5 levels)
int num = 25;
int result = (num > 50) ? 1 : ((num > 40) ? 2 : ((num > 30) ? 3 : ((num > 20) ? 4 : 5)));
print(result);

print("Test passed"); // Test completed
