// ERROR: Static method accessing instance field

class MyClass {
    int instanceField;

    public function MyClass() {
        instanceField = 42;
    }

    // ERROR: Static method cannot access instance field
    public static function staticMethod(): int {
        return instanceField;  // ERROR HERE
    }
}

function main(): void {
    int result = MyClass::staticMethod();
    print("This should not execute");
}

main();
