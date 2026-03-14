// Test string interpolation with arrays
int[] nums = new int[3];
nums[0] = 1;
nums[1] = 2;
nums[2] = 3;
string result = $"nums: {nums}";
print(result);

string[] names = new string[2];
names[0] = "Alice";
names[1] = "Bob";
print($"names: {names}");
