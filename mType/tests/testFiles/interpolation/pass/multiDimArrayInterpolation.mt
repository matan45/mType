// Test string interpolation with multi-dimensional arrays
int[][] grid = new int[2][3];
grid[0][0] = 1;
grid[0][1] = 2;
grid[0][2] = 3;
grid[1][0] = 4;
grid[1][1] = 5;
grid[1][2] = 6;

string result = $"grid: {grid}";
print(result);
