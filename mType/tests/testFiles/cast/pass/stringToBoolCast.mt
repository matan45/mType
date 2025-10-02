// Test: String to Bool casting (truthy check)
string empty = "";
bool b1 = (bool)empty;
print(b1);  // Expected: false

string nonEmpty = "hello";
bool b2 = (bool)nonEmpty;
print(b2);  // Expected: true

string space = " ";
bool b3 = (bool)space;
print(b3);  // Expected: true

// Expected output:
// false
// true
// true
