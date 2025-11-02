// Test 3D array type casting
// Demonstrates type casting with three-dimensional arrays

class Base {
    int value;
}

class Derived extends Base {
    string name;
}

@Script
void test3DArrayCasting() {
    print("Testing 3D array casting");

    // Create 3D array of derived type
    Derived[][][] cube = new Derived[2][2][2];

    // Initialize all elements
    int counter = 1;
    for (int i = 0; i < 2; i = i + 1) {
        for (int j = 0; j < 2; j = j + 1) {
            for (int k = 0; k < 2; k = k + 1) {
                cube[i][j][k] = new Derived();
                cube[i][j][k].value = counter;
                cube[i][j][k].name = "Item" + counter;
                counter = counter + 1;
            }
        }
    }

    print("3D array dimensions: " + cube.length + "x" + cube[0].length + "x" + cube[0][0].length);

    // Upcast to base type
    Base[][][] baseCube = cube;
    print("Value at [0][0][0]: " + baseCube[0][0][0].value);
    print("Value at [1][1][1]: " + baseCube[1][1][1].value);

    // Access through cast
    Derived derived = (Derived)baseCube[0][1][0];
    print("Name at [0][1][0]: " + derived.name);

    derived = (Derived)baseCube[1][0][1];
    print("Name at [1][0][1]: " + derived.name);

    print("3D array casting completed");
}
