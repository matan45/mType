// Test array return types
function getIntArray(): int[] {
    return [1, 2, 3];
}

function getStringArray(): string[] {
    return ["a", "b", "c"];
}

function get2DArray(): int[][] {
    return [[1, 2], [3, 4]];
}

// Test functions
int[] nums = getIntArray();
print(nums[0]);
print(nums[2]);

string[] strs = getStringArray();
print(strs[1]);

int[][] matrix = get2DArray();
print(matrix[1][0]);
