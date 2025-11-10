// Test two interfaces with incompatible signatures
// @Throw

interface Reader {
    func read(): String;
}

interface DataReader {
    func read(): Int;  // Same method name, different return type
}

// Error: Cannot implement both - conflicting return types
class FileReader implements Reader, DataReader {
    func read(): String {
        return "data";
    }

    func read(): Int {
        return 42;
    }
}

var reader = new FileReader();
print(reader.read());
