// MYT-281: arrays widen to Object — basic assignment.
print("Testing array assigned to Object slot");

int[] nums = new int[3];
nums[0] = 10;
nums[1] = 20;
nums[2] = 30;

Object x = nums;

// The receiver is still array-shaped at runtime; reach back through cast.
int[] back = (int[])x;
print("First element: " + back[0]);
print("Length: " + back.length);
print("Done");
