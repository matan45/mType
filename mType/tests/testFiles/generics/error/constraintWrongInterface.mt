// Test: Class implements wrong interface
// Expected: Should fail - class implements different interface than required

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Serializable {
    function serialize(): string;
}

class Document implements Serializable {
    private string content;

    constructor(string c) {
        this.content = c;
    }

    function serialize(): string {
        return this.content;
    }
    // Implements Serializable but NOT Comparable
}

class SortedList<T extends Comparable<T>> {
    private List<T> items;

    constructor() {
        this.items = new List<T>();
    }
}

// ERROR: Document implements Serializable, not Comparable<Document>
SortedList<Document> list = new SortedList<Document>();
