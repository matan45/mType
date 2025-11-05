// Test interface with overloaded methods
// @Script

interface Printer {
    function print(String message): void;
    function print(Int value): void;
    function print(Bool flag): void;
}

class ConsolePrinter implements Printer {
    public function print(String message): void {
        print("[String] " + message);
    }

    public function print(Int value): void {
        print("[Int] " + value);
    }

    public function print(Bool flag): void {
        String msg = "true";
        if (!flag) {
            msg = "false";
        }
        print("[Bool] " + msg);
    }
}

ConsolePrinter printer = new ConsolePrinter();
printer.print("Hello");
printer.print(42);
printer.print(true);
