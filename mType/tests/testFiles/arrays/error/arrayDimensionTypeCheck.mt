// Test type checking across dimensions (should error)
print("Testing dimension type check");

int[][] matrix = new int[2][];
matrix[0] = new int[3];
matrix[1] = new int[3];

// Try to assign string array to int[][] (should error)
string[] wrongType = new string[3];
matrix[0] = wrongType;

print("This should not be reached");
