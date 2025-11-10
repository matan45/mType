// Test single-element array edge cases
print("Testing single-element arrays");

// Create single-element arrays of different types
int[] singleInt = new int[1];
singleInt[0] = 42;
print("Single int array: [" + singleInt[0] + "]");

string[] singleString = new string[1];
singleString[0] = "hello";
print("Single string array: [" + singleString[0] + "]");

bool[] singleBool = new bool[1];
singleBool[0] = true;
print("Single bool array: [" + singleBool[0] + "]");

// Test loop over single element
int iterations = 0;
for (int i = 0; i < singleInt.length; i++) {
    iterations = iterations + 1;
    print("Loop iteration " + iterations + ": singleInt[" + i + "] = " + singleInt[i]);
}

// Test modification
print("Before modification: " + singleInt[0]);
singleInt[0] = 100;
print("After modification: " + singleInt[0]);

// Test with object type
class Box {
    int value;

    constructor(int v) {
        value = v;
    }

    public function getValue(): int {
        return value;
    }
}

Box[] singleBox = new Box[1];
singleBox[0] = new Box(99);
print("Single Box array value: " + singleBox[0].getValue());

// Test multidimensional with single elements
int[][] singleByDimension = new int[1][];
singleByDimension[0] = new int[1];
singleByDimension[0][0] = 7;
print("Single element in 2D array: " + singleByDimension[0][0]);

print("Single-element arrays test completed");
