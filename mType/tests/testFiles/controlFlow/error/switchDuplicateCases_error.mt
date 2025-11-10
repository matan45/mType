// Switch statement with duplicate case values - should produce error
// Duplicate cases violate switch statement semantics

int value = 2;
switch (value) {
    case 1:
        print("First case 1");
        break;
    case 2:
        print("First case 2");
        break;
    case 2:  // ERROR: Duplicate case value
        print("Second case 2 - duplicate");
        break;
    case 3:
        print("Case 3");
        break;
    default:
        print("Default");
        break;
}

print("This should not execute");
