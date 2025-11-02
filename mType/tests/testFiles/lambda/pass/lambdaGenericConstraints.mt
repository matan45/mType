// Lambda with interface-bound generics
interface Printable {
    function toString() : String;
}

interface Function<T, R> {
    function apply(T input) : R;
}

class Document implements Printable {
    String title;

    function init(String t) {
        this.title = t;
    }

    function toString() : String {
        return "Document: " + this.title;
    }
}

class Report implements Printable {
    int pages;

    function init(int p) {
        this.pages = p;
    }

    function toString() : String {
        return "Report with " + pages + " pages";
    }
}

print("=== Generic Constraints Test ===");

// Lambda accepting anything that implements Printable
Function<Printable, String> printer = p -> "Printing: " + p.toString();

Document doc = new Document("Manual");
Report rep = new Report(50);

print(printer.apply(doc));
print(printer.apply(rep));

// Lambda with constrained type processing
Function<Printable, int> lengthCounter = p -> {
    String s = p.toString();
    return s.length();
};

print("Doc string length: " + lengthCounter.apply(doc));
print("Report string length: " + lengthCounter.apply(rep));

print("Generic constraints complete");
