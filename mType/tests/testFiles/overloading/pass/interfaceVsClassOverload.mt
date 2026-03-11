// Test: Overload resolution with interface type vs concrete class type
interface Printable {
    function show(): string;
}

class Document implements Printable {
    string content;

    constructor(string c) {
        this.content = c;
    }

    public function show(): string {
        return this.content;
    }
}

function display(Document d): string {
    return "Document: " + d.show();
}

function display(Printable p): string {
    return "Printable: " + p.show();
}

function main(): void {
    print("=== Interface vs Class Overload ===");

    Document doc = new Document("hello");
    // Document is exact class match, should win over interface match
    print(display(doc));
}
main();
