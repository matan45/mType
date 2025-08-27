class MyClass {
    int value;
}

function processObject(MyClass obj): void {
    print(toString(obj.value));
}

processObject(true);  // Should fail: bool passed where object expected