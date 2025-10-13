// Test Structure-of-Arrays (SoA) optimized object arrays
// Arrays with size >= 16 automatically use SoA storage for 40-85% memory reduction
// Each field is stored in a separate SIMD-optimized array (field-oriented)

// ============================================
// Test Classes for SoA Optimization
// ============================================

class Person {
    public int id;
    public string name;
    public int age;
    public float salary;

    public constructor(int id, string name, int age, float salary) {
        this.id = id;
        this.name = name;
        this.age = age;
        this.salary = salary;
    }
}

class Point2D {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

class Particle {
    public float x;
    public float y;
    public float z;
    public float vx;
    public float vy;
    public float vz;
    public bool active;

    public constructor(float x, float y, float z, float vx, float vy, float vz, bool active) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.vx = vx;
        this.vy = vy;
        this.vz = vz;
        this.active = active;
    }
}

// ============================================
// 1D SoA Object Arrays
// ============================================

// Test 1.1: Person Array (1D) - 20 elements with SoA
// Memory layout: id[], name[], age[], salary[] (field-oriented)
Person[] people = new Person[20];
for (int i = 0; i < 20; i = i + 1) {
    people[i] = new Person(1000 + i, "Person" + i, 20 + i, 50000.0 + i * 1000.0);
}

// Verify SoA object array access
print("Person[0] ID: " + people[0].id);         // Expected: 1000
print("Person[0] Name: " + people[0].name);     // Expected: Person0
print("Person[5] Age: " + people[5].age);       // Expected: 25
print("Person[10] Salary: " + people[10].salary); // Expected: 60000.0

// Test 1.2: Calculate average age (field access is cache-friendly with SoA)
int totalAge = 0;
for (int i = 0; i < 20; i = i + 1) {
    totalAge = totalAge + people[i].age;
}
float avgAge = totalAge / 20;
print("Average Age: " + avgAge); // Expected: 29.5

// Test 1.3: Point2D Array (1D) - 25 elements with SoA
Point2D[] points = new Point2D[25];
for (int i = 0; i < 25; i = i + 1) {
    points[i] = new Point2D(i * 10, i * 20);
}

print("Point[10] X: " + points[10].x); // Expected: 100
print("Point[10] Y: " + points[10].y); // Expected: 200

// Test 1.4: Small object array (< 16 elements) - No SoA optimization
Person[] smallPeople = new Person[10];
for (int i = 0; i < 10; i = i + 1) {
    smallPeople[i] = new Person(i, "Small" + i, 30 + i, 40000.0);
}
print("Small Person[5] ID: " + smallPeople[5].id); // Expected: 5

// ============================================
// 2D SoA Object Arrays - FlatMultiObjectArray
// ============================================

// Test 2.1: Person Array (2D) - 4x5 = 20 elements with SoA
// Memory layout: Single flat array per field (id[20], name[20], age[20], salary[20])
Person[][] peopleGrid = new Person[4][5];
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        int idx = i * 5 + j;
        peopleGrid[i][j] = new Person(2000 + idx, "Grid" + idx, 25 + idx, 55000.0 + idx * 500.0);
    }
}

print("PeopleGrid[2][3] ID: " + peopleGrid[2][3].id);       // Expected: 2013
print("PeopleGrid[2][3] Name: " + peopleGrid[2][3].name);   // Expected: Grid13
print("PeopleGrid[2][3] Age: " + peopleGrid[2][3].age);     // Expected: 38

// Test 2.2: Point2D Array (2D) - 5x4 = 20 elements
Point2D[][] pointsGrid = new Point2D[5][4];
for (int i = 0; i < 5; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        pointsGrid[i][j] = new Point2D(i * 100 + j * 10, i * 200 + j * 20);
    }
}

print("PointsGrid[3][2] X: " + pointsGrid[3][2].x); // Expected: 320
print("PointsGrid[3][2] Y: " + pointsGrid[3][2].y); // Expected: 640

// Test 2.3: Calculate total salary in 2D grid
float totalSalary = 0.0;
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        totalSalary = totalSalary + peopleGrid[i][j].salary;
    }
}
print("Total Salary (2D Grid): " + totalSalary); // Expected: 1195000.0

// ============================================
// 3D SoA Object Arrays - High-dimensional FlatMultiObjectArray
// ============================================

// Test 3.1: Point2D Array (3D) - 3x3x3 = 27 elements (>= 16, uses SoA)
Point2D[][][] pointsCube = new Point2D[3][3][3];
for (int i = 0; i < 3; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
        for (int k = 0; k < 3; k = k + 1) {
            int linearIdx = i * 9 + j * 3 + k;
            pointsCube[i][j][k] = new Point2D(linearIdx, linearIdx * 10);
        }
    }
}

print("PointsCube[1][2][1] X: " + pointsCube[1][2][1].x); // Expected: 16
print("PointsCube[1][2][1] Y: " + pointsCube[1][2][1].y); // Expected: 160

// Test 3.2: Person Array (3D) - 2x3x4 = 24 elements
Person[][][] peopleCube = new Person[2][3][4];
for (int i = 0; i < 2; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
        for (int k = 0; k < 4; k = k + 1) {
            int linearIdx = i * 12 + j * 4 + k;
            peopleCube[i][j][k] = new Person(3000 + linearIdx, "Cube" + linearIdx,
                                             30 + linearIdx, 60000.0 + linearIdx * 1000.0);
        }
    }
}

print("PeopleCube[1][1][2] ID: " + peopleCube[1][1][2].id);   // Expected: 3018
print("PeopleCube[1][1][2] Name: " + peopleCube[1][1][2].name); // Expected: Cube18
print("PeopleCube[1][1][2] Age: " + peopleCube[1][1][2].age);  // Expected: 48

// Test 3.3: Count people over 40 in 3D cube
int over40Count = 0;
for (int i = 0; i < 2; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
        for (int k = 0; k < 4; k = k + 1) {
            if (peopleCube[i][j][k].age > 40) {
                over40Count = over40Count + 1;
            }
        }
    }
}
print("People Over 40 Count (3D): " + over40Count); // Expected: 13

// ============================================
// Large SoA Object Arrays - Maximum Memory Benefit
// ============================================

// Test 4.1: Large Person array - 100 elements
// Memory savings: ~85% compared to Array-of-Structures
Person[] largePeople = new Person[100];
for (int i = 0; i < 100; i = i + 1) {
    largePeople[i] = new Person(5000 + i, "Large" + i, 25 + (i % 40), 50000.0 + i * 500.0);
}

// Access and verify
print("LargePeople[50] ID: " + largePeople[50].id);     // Expected: 5050
print("LargePeople[50] Age: " + largePeople[50].age);   // Expected: 35
print("LargePeople[99] Name: " + largePeople[99].name); // Expected: Large99

// Test 4.2: Particle array - 50 elements with 7 fields
// Each field stored in separate SIMD-optimized array
Particle[] particles = new Particle[50];
for (int i = 0; i < 50; i = i + 1) {
    particles[i] = new Particle(
        i * 1.0,           // x
        i * 2.0,           // y
        i * 3.0,           // z
        i * 0.1,           // vx
        i * 0.2,           // vy
        i * 0.3,           // vz
        (i % 2 == 0)       // active
    );
}

// Count active particles (field access benefits from SoA)
int activeCount = 0;
for (int i = 0; i < 50; i = i + 1) {
    if (particles[i].active) {
        activeCount = activeCount + 1;
    }
}
print("Active Particles Count: " + activeCount); // Expected: 25

// Calculate average X position
float totalX = 0.0;
for (int i = 0; i < 50; i = i + 1) {
    totalX = totalX + particles[i].x;
}
float avgX = totalX / 50.0;
print("Average X Position: " + avgX); // Expected: 24.5

print("SoA Object Arrays Test Completed!");
