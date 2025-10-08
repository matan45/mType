// Pass test: Multiple classes can implement a final interface
// Only extension is blocked, not implementation

final interface Printable {
    function doSomething(): void;
}

class Document implements Printable {
    string content;

    constructor(string c) {
        content = c;
    }

    public function doSomething(): void {
        print(content);
    }
}

class Report implements Printable {
    int reportId;

    constructor(int id) {
        reportId = id;
    }

    public function doSomething(): void {
        print(reportId);
    }
}

Document doc = new Document("Hello World");
doc.doSomething();

Report rep = new Report(123);
rep.doSomething();
