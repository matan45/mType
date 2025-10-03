// Test: Interface methods default to public
interface Printable {
    void display();  // No modifier = public by default for interfaces
}

class Document implements Printable {
    public string title;

    public Document(string t) {
        title = t;
    }

    public void display() {
        print("Document: " + title);
    }
}

Document doc = new Document("Report");
doc.display();  // Expected: Document: Report
