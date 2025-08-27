// Test enhanced null pointer exception with location info
class Person {
    string name;
}

Person p = null;
print(p.name);  // Line 6: Should show null pointer access error with location
print("This should not execute");