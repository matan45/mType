// Test array type parameters
function testIntArray(int[] arr): int {
    return arr[0];
}

function testStringArray(string[] strs): string {
    return strs[1];
}

function test2DArray(int[][] matrix): int {
    return matrix[0][0];
}

function testMultipleArrays(int[] nums, string[] words): void {
    print(nums[0]);
    print(words[0]);
}

// Call functions
int[] numbers = [1, 2, 3];
print(testIntArray(numbers));

string[] words = ["hello", "world"];
print(testStringArray(words));

int[][] matrix = [[10, 20], [30, 40]];
print(test2DArray(matrix));

testMultipleArrays(numbers, words);
