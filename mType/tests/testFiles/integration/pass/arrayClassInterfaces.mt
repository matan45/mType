// Arrays + Class Test 3: Object arrays with interfaces
@Script

interface Printable {
    fun print(): Void;
}

interface Serializable {
    fun serialize(): String;
}

class Document implements Printable, Serializable {
    field title: String;
    field content: String;

    constructor(t: String, c: String) {
        this.title = t;
        this.content = c;
    }

    fun print(): Void {
        print(this.title);
        print(this.content);
    }

    fun serialize(): String {
        return this.title;
    }
}

class Report extends Document {
    field reportId: Int;

    constructor(id: Int, t: String, c: String) {
        super(t, c);
        this.reportId = id;
    }

    fun serialize(): String {
        return this.title;
    }

    fun getId(): Int {
        return this.reportId;
    }
}

class Image implements Printable {
    field filename: String;
    field width: Int;
    field height: Int;

    constructor(f: String, w: Int, h: Int) {
        this.filename = f;
        this.width = w;
        this.height = h;
    }

    fun print(): Void {
        print(this.filename);
        print(this.width);
        print(this.height);
    }
}

print("Creating Printable array:");
let printables: Printable[] = Printable[4];
printables[0] = Document("Doc1", "Content1");
printables[1] = Image("img.png", 800, 600);
printables[2] = Report(101, "Report1", "Data");
printables[3] = Document("Doc2", "Content2");

print("Printing all:");
let i: Int = 0;
while (i < 4) {
    printables[i].print();
    i = i + 1;
}

print("Creating Document array:");
let docs: Document[] = Document[3];
docs[0] = Document("First", "Text1");
docs[1] = Report(201, "Second", "Text2");
docs[2] = Document("Third", "Text3");

print("Serializing documents:");
i = 0;
while (i < 3) {
    print(docs[i].serialize());
    i = i + 1;
}

print("Creating Report array:");
let reports: Report[] = Report[2];
reports[0] = Report(301, "Q1", "Quarterly");
reports[1] = Report(302, "Q2", "Quarterly");

print("Report details:");
i = 0;
while (i < 2) {
    print(reports[i].getId());
    reports[i].print();
    i = i + 1;
}
