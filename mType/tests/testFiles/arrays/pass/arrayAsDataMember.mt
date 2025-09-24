// Test basic array functionality (standalone)
print("Testing arrays as class data members");

// Test array operations with improved syntax
int[] grades = new int[3];
grades[0] = 85;
grades[1] = 92;
grades[2] = 78;

print("Grades array length: " + grades.length);
print("Math grade: " + grades[0]);
print("Science grade: " + grades[1]);
print("English grade: " + grades[2]);

// Test string arrays with loops
string[] subjects = new string[3];
subjects[0] = "Math";
subjects[1] = "Science";
subjects[2] = "English";

print("Subjects array length: " + subjects.length);
for (int i = 0; i < subjects.length; i++) {
    print("Subject " + i + ": " + subjects[i]);
}

print("Array as data member test completed");