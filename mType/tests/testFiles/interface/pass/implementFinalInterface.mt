// Pass test: Multiple classes can implement a final interface
// Only extension is blocked, not implementation

final interface Printable {
    function doSomething(): void;
}

class Document implements Printable {
    private string content;

    public constructor(string c) {
        this.content = c;
    }

    public function doSomething(): void {
        print(this.content);
    }
}

class Report implements Printable {
    private int reportId;

    public constructor(int id) {
        this.reportId = id;
    }

    public function doSomething(): void {
        print(this.reportId);
    }
}

Document doc = new Document("Hello World");
doc.doSomething();

Report rep = new Report(123);
rep.doSomething();
