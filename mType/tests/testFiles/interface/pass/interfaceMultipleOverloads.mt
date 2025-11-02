// Test multiple interfaces with same method name but different signatures
// @Script

interface StringProcessor {
    func process(input: String): String;
}

interface IntProcessor {
    func process(input: Int): Int;
}

class UniversalProcessor implements StringProcessor, IntProcessor {
    func process(input: String): String {
        return input + "!";
    }

    func process(input: Int): Int {
        return input * 2;
    }
}

var processor = new UniversalProcessor();

var str = processor.process("Hello");
print(str);  // Should print "Hello!"

var num = processor.process(21);
print(num);  // Should print 42

// Can be used as either interface type
var sp: StringProcessor = processor;
print(sp.process("World"));  // Should print "World!"

var ip: IntProcessor = processor;
print(ip.process(10));       // Should print 20
