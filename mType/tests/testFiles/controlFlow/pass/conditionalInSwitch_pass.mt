// Test conditionals as switch expressions

// Test 1: If-else in switch cases
int x = 2;
switch (x) {
    case 1:
        if (true) {
            print(100);
        } else {
            print(101);
        }
        break;
    case 2:
        if (false) {
            print(200);
        } else {
            print(201);
        }
        break;
    case 3:
        print(300);
        break;
    default:
        print(999);
        break;
}

// Test 2: Nested conditionals in switch
int y = 1;
switch (y) {
    case 1:
        if (true) {
            if (true) {
                print(400);
            } else {
                print(401);
            }
        } else {
            print(402);
        }
        break;
    case 2:
        print(500);
        break;
    default:
        print(999);
        break;
}

// Test 3: Complex expressions in switch with conditionals
int z = 3;
switch (z) {
    case 1:
        print(600);
        break;
    case 2:
        print(700);
        break;
    case 3:
        int temp = 10;
        if (temp > 5) {
            print(800);
        } else {
            print(801);
        }
        break;
    default:
        if (z > 10) {
            print(900);
        } else {
            print(901);
        }
        break;
}

// Test 4: Ternary in switch
int w = 1;
switch (w) {
    case 1:
        int result = w == 1 ? 1000 : 1001;
        print(result);
        break;
    case 2:
        print(1100);
        break;
    default:
        print(1200);
        break;
}

print("Test passed"); // Test completed
