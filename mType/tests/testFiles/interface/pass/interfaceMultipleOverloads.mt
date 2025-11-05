// Test multiple interfaces with same method name but different signatures
// @Script

interface StringProcessor {
    function process(string input): string;
}

interface IntProcessor {
    function process(int input): int;
}

class UniversalProcessor implements StringProcessor, IntProcessor {
    public function process(string input): string {
        return input + "!";
    }

    public function process(int input): int {
        return input * 2;
    }
}

UniversalProcessor processor = new UniversalProcessor();

string str = processor.process("Hello");
print(str);  // Should print "Hello!"

int num = processor.process(21);
print(num);  // Should print 42

// Can be used as either interface type
StringProcessor sp = processor;
print(sp.process("World"));  // Should print "World!"

IntProcessor ip = processor;
print(ip.process(10));       // Should print 20
