// Test interface with overloaded methods
// @Script

interface Printer {
    function printString(string message): void;
    function printInt(int value): void;
    function printBool(bool flag): void;
}

class ConsolePrinter implements Printer {
    public function printString(string message): void {
        print("[String] " + message);
    }

    public function printInt(int value): void {
        print("[Int] " + value);
    }

    public function printBool(bool flag): void {
        string msg = "true";
        if (!flag) {
            msg = "false";
        }
        print("[Bool] " + msg);
    }
}

ConsolePrinter printer = new ConsolePrinter();
printer.printString("Hello");
printer.printInt(42);
printer.printBool(true);
