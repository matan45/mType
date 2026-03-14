// Lambda with interface-bound generics
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Printable {
    function toString() : string;
}

interface Function<T, R> {
    function apply(T input) : R;
}

class Document implements Printable {
    String title;

    constructor(String t) {
        this.title = t;
    }

    public function toString() : string {
        return "Document: " + this.title.getValue();
    }
}

class Report implements Printable {
    int pages;

    constructor(int p) {
        this.pages = p;
    }

    public function toString() : string {
        return "Report with " + pages + " pages";
    }
}

print("=== Generic Constraints Test ===");

// Lambda accepting anything that implements Printable
Function<Printable, String> printer = p -> new String("Printing: " + p.toString());

Document doc = new Document(new String("Manual"));
Report rep = new Report(50);

print(printer.apply(doc).getValue());
print(printer.apply(rep).getValue());

// Lambda with constrained type processing
Function<Printable, Int> lengthCounter = p -> {
    string s = p.toString();
    return new Int(strLength(s));
};

print("Doc string length: " + lengthCounter.apply(doc).getValue());
print("Report string length: " + lengthCounter.apply(rep).getValue());

print("Generic constraints complete");
