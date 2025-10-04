// Test: Method call after interface cast
interface Printable {
    function print(): void;
}

class Document implements Printable {
    public string title = "Report";

    public function print(): void {
        print(this.title);
    }
}

Document doc = new Document();
Printable p = (Printable)doc;
p.print();

// Expected output:
// Report
