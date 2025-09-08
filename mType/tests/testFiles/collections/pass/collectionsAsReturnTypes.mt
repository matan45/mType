// Simple collection test without function parameters

Array<int> numbers = [1, 2, 3, 4, 5];
print("Array size:", numbers.size());

for (int num : numbers) {
    print("Number:", num);
}

Map<string, int> grades = {"Alice": 95, "Bob": 87};
print("Map size:", grades.size());

for (int grade : grades) {
    print("Grade:", grade);
}

print("Simple collection test passed");