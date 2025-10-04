// Test: Interface methods default to public
interface Printable {
    function display(): void;  // No modifier = public by default for interfaces
}

class Document implements Printable {
    public string title;

    public constructor(string t) {
        title = t;
    }

    public function display(): void {
        print("Document: " + title);
    }
}

Document doc = new Document("Report");
doc.display();  // Expected: Document: Report
