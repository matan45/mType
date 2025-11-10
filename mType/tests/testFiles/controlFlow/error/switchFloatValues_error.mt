// Switch statement with float/double values - should produce error
// Floating point comparison in switch is typically not supported due to precision issues

float value = 3.14;
switch (value) {
    case 3.14:
        print("Pi approximation");
        break;
    case 2.71:
        print("Euler's number approximation");
        break;
    case 1.41:
        print("Square root of 2");
        break;
    default:
        print("Unknown value");
        break;
}

print("This should not execute");
