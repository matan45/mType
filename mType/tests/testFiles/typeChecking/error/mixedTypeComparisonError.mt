// ERROR: Invalid mixed type comparison

function main(): void {
    int intValue = 42;
    string strValue = "42";

    // ERROR: Cannot compare int and string
    bool result = (intValue == strValue);

    print("This should not execute");
}

main();
