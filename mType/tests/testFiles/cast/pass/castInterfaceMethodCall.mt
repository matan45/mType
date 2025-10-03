// Test: Method call after interface cast
interface Printable {
    function print(): void;
}

class Document implements Printable {
    string title = "Report";

    function print(): void {
        print(this.title);
    }
}

Document doc = new Document();
Printable p = (Printable)doc;
p.print();

// Expected output:
// Report
