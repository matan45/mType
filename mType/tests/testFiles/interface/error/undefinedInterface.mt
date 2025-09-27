// Implementing a non-existent interface
class MyClass implements NonExistentInterface {
    function someMethod(): void {
        print("Some method");
    }
}

print("This should not print");