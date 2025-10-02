// Test: Method call after interface cast
interface Printable {
    void print();
}

class Document implements Printable {
    string title = "Report";

    void print() {
        print(this.title);
    }
}

Document doc = new Document();
Printable p = (Printable)doc;
p.print();

// Expected output:
// Report
