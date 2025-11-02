// Test interface with overloaded methods
// @Script

interface Printer {
    func print(message: String): void;
    func print(value: Int): void;
    func print(flag: Bool): void;
}

class ConsolePrinter implements Printer {
    func print(message: String): void {
        print("[String] " + message);
    }

    func print(value: Int): void {
        print("[Int] " + value.toString());
    }

    func print(flag: Bool): void {
        var msg = "true";
        if (!flag) {
            msg = "false";
        }
        print("[Bool] " + msg);
    }
}

var printer = new ConsolePrinter();
printer.print("Hello");
printer.print(42);
printer.print(true);
