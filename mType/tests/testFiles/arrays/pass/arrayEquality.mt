// Test content-based array equality
print("Testing array equality");

int[] arr1 = new int[3];
arr1[0] = 10;
arr1[1] = 20;
arr1[2] = 30;

int[] arr2 = new int[3];
arr2[0] = 10;
arr2[1] = 20;
arr2[2] = 30;

int[] arr3 = new int[3];
arr3[0] = 10;
arr3[1] = 25;
arr3[2] = 30;

int[] arr4 = new int[2];
arr4[0] = 10;
arr4[1] = 20;

// Manual content equality check
function arraysEqual(int[] a, int[] b): bool {
    if (a.length != b.length) {
        return false;
    }
    for (int i = 0; i < a.length; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

print("arr1 equals arr2: " + arraysEqual(arr1, arr2));
print("arr1 equals arr3: " + arraysEqual(arr1, arr3));
print("arr1 equals arr4: " + arraysEqual(arr1, arr4));

// Test reference equality
bool sameReference = (arr1 == arr1);
print("arr1 == arr1 (same reference): " + sameReference);

// Test with string arrays
string[] strArr1 = new string[2];
strArr1[0] = "hello";
strArr1[1] = "world";

string[] strArr2 = new string[2];
strArr2[0] = "hello";
strArr2[1] = "world";

function stringArraysEqual(string[] a, string[] b): bool {
    if (a.length != b.length) {
        return false;
    }
    for (int i = 0; i < a.length; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

print("strArr1 equals strArr2: " + stringArraysEqual(strArr1, strArr2));

print("Array equality test completed");
