// Test return statements from inside if/for/while blocks
function findFirstEven(int[] arr): int {
    for (int i = 0; i < arr.length; i = i + 1) {
        if (arr[i] % 2 == 0) {
            return arr[i];
        }
    }
    return -1;
}

function findFirstPositive(int[] arr): int {
    int i = 0;
    while (i < arr.length) {
        if (arr[i] > 0) {
            return arr[i];
        }
        i = i + 1;
    }
    return -1;
}

function checkConditions(int x, int y): string {
    if (x > 0) {
        if (y > 0) {
            return "both positive";
        } else {
            return "x positive only";
        }
    } else {
        if (y > 0) {
            return "y positive only";
        } else {
            return "both non-positive";
        }
    }
}

function searchMatrix(int[][] matrix, int target): bool {
    for (int i = 0; i < matrix.length; i = i + 1) {
        for (int j = 0; j < matrix[i].length; j = j + 1) {
            if (matrix[i][j] == target) {
                return true;
            }
        }
    }
    return false;
}

int[] nums1 = [1, 3, 5, 8, 9];
print(findFirstEven(nums1));  // 8

int[] nums2 = [-5, -2, 3, 7];
print(findFirstPositive(nums2));  // 3

print(checkConditions(5, 10));   // both positive
print(checkConditions(5, -2));   // x positive only
print(checkConditions(-3, 8));   // y positive only
print(checkConditions(-1, -4));  // both non-positive

int[][] matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
print(searchMatrix(matrix, 5));   // true
print(searchMatrix(matrix, 10));  // false

print("Test passed"); // Test completed
