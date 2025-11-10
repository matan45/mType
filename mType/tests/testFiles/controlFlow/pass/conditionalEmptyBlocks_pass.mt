// Test if/else with empty bodies
int counter = 0;

// Empty if block
if (false) {
}
print(100);

// Empty else block
if (true) {
    print(200);
} else {
}

// Empty if with non-empty else
if (false) {
} else {
    print(300);
}

// Multiple empty blocks in else-if chain
int x = 3;
if (x == 1) {
} else if (x == 2) {
} else if (x == 3) {
    print(400);
} else {
}

// Empty blocks with side effects before/after
print(500);
if (true) {
}
print(600);

if (false) {
} else {
}
print(700);

// Nested empty blocks
if (true) {
    if (false) {
    } else {
    }
    print(800);
}

// Empty block with complex condition
if (1 + 1 == 2 && 3 > 2) {
}
print(900);

print("Test passed"); // Test completed
