interface Printable {
    function display(): string;
}

class Document implements Printable {
    string title;

    public constructor(string title) {
        this.title = title;
    }

    public function display(): string {
        return "Document: " + this.title;
    }
}

Document doc = new Document("Report");
print(doc.display());
Printable p = doc;
print(p.display());
