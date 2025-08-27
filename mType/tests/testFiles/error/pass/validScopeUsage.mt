int x = 10;

if (true) {
    int y = 20;
    print(x); // x is in scope
    print(y); // y is in scope
}

function test(): int {
    int localVar = 30;
    return localVar;
}

int result = test();
print(result);
print("Test passed");