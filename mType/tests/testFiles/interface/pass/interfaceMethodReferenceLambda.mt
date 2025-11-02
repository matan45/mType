// Test method reference to interface method (if supported)
// @Script

interface Printer {
    func print(message: String): void;
}

class ConsolePrinter implements Printer {
    var prefix: String;

    func init(prefix: String) {
        this.prefix = prefix;
    }

    func print(message: String): void {
        print(this.prefix + message);
    }
}

class MessageProcessor {
    func process(messages: Array<String>, printer: Printer): void {
        var i = 0;
        while (i < messages.size()) {
            printer.print(messages.get(i));
            i = i + 1;
        }
    }
}

var messages = new Array<String>();
messages.add("Hello");
messages.add("World");
messages.add("Test");

var processor = new MessageProcessor();
var consolePrinter = new ConsolePrinter("[INFO] ");

// Pass interface implementation
processor.process(messages, consolePrinter);

// Alternative with different printer
var errorPrinter = new ConsolePrinter("[ERROR] ");
processor.process(messages, errorPrinter);
