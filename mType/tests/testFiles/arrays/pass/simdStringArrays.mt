// Test SIMD-optimized string arrays with StringPool
// Arrays with size >= 16 use StringPool for 50-80% memory reduction
// Strings are deduplicated and stored as pool IDs (32-bit integers)

// ============================================
// 1D String Arrays - StringPool Optimization
// ============================================

// Test 1.1: String Array (1D) - 20 elements with deduplication
string[] names = new string[20];
names[0] = "Alice";
names[1] = "Bob";
names[2] = "Charlie";
names[3] = "Alice";  // Duplicate - reuses pool ID
names[4] = "Bob";    // Duplicate - reuses pool ID
names[5] = "David";
names[6] = "Eve";
names[7] = "Charlie"; // Duplicate
names[8] = "Alice";   // Duplicate
names[9] = "Frank";
names[10] = "Grace";
names[11] = "Bob";    // Duplicate
names[12] = "Alice";  // Duplicate
names[13] = "Helen";
names[14] = "Ivan";
names[15] = "Charlie"; // Duplicate
names[16] = "Alice";   // Duplicate
names[17] = "Bob";     // Duplicate
names[18] = "Jack";
names[19] = "Alice";   // Duplicate

// Verify string pool deduplication works
print("Name[0]: " + names[0]);   // Expected: Alice
print("Name[3]: " + names[3]);   // Expected: Alice (same pool ID)
print("Name[11]: " + names[11]); // Expected: Bob

// Test 1.2: String Array with operations
string[] cities = new string[25];
for (int i = 0; i < 25; i = i + 1) {
    if (i % 5 == 0) {
        cities[i] = "New York";
    }
    if (i % 5 == 1) {
        cities[i] = "London";
    }
    if (i % 5 == 2) {
        cities[i] = "Tokyo";
    }
    if (i % 5 == 3) {
        cities[i] = "Paris";
    }
    if (i % 5 == 4) {
        cities[i] = "Sydney";
    }
}

// Count occurrences (tests string comparison with pool IDs)
int newYorkCount = 0;
for (int i = 0; i < 25; i = i + 1) {
    if (cities[i] == "New York") {
        newYorkCount = newYorkCount + 1;
    }
}
print("New York Count: " + newYorkCount); // Expected: 5

// Test 1.3: Small string array (< 16 elements) - No StringPool
string[] smallStrings = new string[10];
for (int i = 0; i < 10; i = i + 1) {
    smallStrings[i] = "Item " + i;
}
print("Small String [5]: " + smallStrings[5]); // Expected: Item 5

// ============================================
// 2D String Arrays - Multi-dimensional StringPool
// ============================================

// Test 2.1: 2D String Array - 5x5 = 25 elements (uses StringPool)
string[][] grid = new string[5][5];
for (int i = 0; i < 5; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        if ((i + j) % 3 == 0) {
            grid[i][j] = "Red";
        }
        if ((i + j) % 3 == 1) {
            grid[i][j] = "Green";
        }
        if ((i + j) % 3 == 2) {
            grid[i][j] = "Blue";
        }
    }
}

print("Grid[2][1]: " + grid[2][1]); // Expected: Red or Green or Blue
print("Grid[0][0]: " + grid[0][0]); // Expected: Red

// Test 2.2: 2D String Array with high deduplication
string[][] departments = new string[4][6];
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 6; j = j + 1) {
        if (j < 2) {
            departments[i][j] = "Engineering";
        }
        if (j >= 2 && j < 4) {
            departments[i][j] = "Marketing";
        }
        if (j >= 4) {
            departments[i][j] = "Sales";
        }
    }
}

// Count departments
int engineeringCount = 0;
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 6; j = j + 1) {
        if (departments[i][j] == "Engineering") {
            engineeringCount = engineeringCount + 1;
        }
    }
}
print("Engineering Count: " + engineeringCount); // Expected: 8

// ============================================
// 3D String Arrays - High-dimensional StringPool
// ============================================

// Test 3.1: 3D String Array - 3x3x3 = 27 elements
string[][][] cube = new string[3][3][3];
for (int i = 0; i < 3; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
        for (int k = 0; k < 3; k = k + 1) {
            if (i == 0) {
                cube[i][j][k] = "Layer0";
            }
            if (i == 1) {
                cube[i][j][k] = "Layer1";
            }
            if (i == 2) {
                cube[i][j][k] = "Layer2";
            }
        }
    }
}

print("Cube[1][1][1]: " + cube[1][1][1]); // Expected: Layer1

// Count Layer1
int layer1Count = 0;
for (int i = 0; i < 3; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
        for (int k = 0; k < 3; k = k + 1) {
            if (cube[i][j][k] == "Layer1") {
                layer1Count = layer1Count + 1;
            }
        }
    }
}
print("Layer1 Count: " + layer1Count); // Expected: 9

// ============================================
// Large String Arrays - Maximum StringPool Benefit
// ============================================

// Test 4.1: Large string array with repetition - 100 elements
string[] statuses = new string[100];
for (int i = 0; i < 100; i = i + 1) {
    if (i % 4 == 0) {
        statuses[i] = "Active";
    }
    if (i % 4 == 1) {
        statuses[i] = "Pending";
    }
    if (i % 4 == 2) {
        statuses[i] = "Completed";
    }
    if (i % 4 == 3) {
        statuses[i] = "Cancelled";
    }
}

// Count active statuses
int activeCount = 0;
for (int i = 0; i < 100; i = i + 1) {
    if (statuses[i] == "Active") {
        activeCount = activeCount + 1;
    }
}
print("Active Status Count: " + activeCount); // Expected: 25

// Test 4.2: String array with unique strings (no deduplication benefit)
string[] uniqueIds = new string[20];
for (int i = 0; i < 20; i = i + 1) {
    uniqueIds[i] = "ID_" + i;
}
print("Unique ID [10]: " + uniqueIds[10]); // Expected: ID_10

print("SIMD String Arrays Test Completed!");
