// Test generic parameters with extends constraints
interface Printable {
    function toString(): string;
}

class Book implements Printable {
    string title;

    constructor(string t) {
        title = t;
    }

    public function toString(): string {
        return title;
    }
}

class Magazine implements Printable {
    string name;

    constructor(string n) {
        name = n;
    }

    public function toString(): string {
        return name;
    }
}

// Generic function with extends constraint
function <T extends Printable> printItem(T item): void {
    print(item.toString());
}

// Call with different types
Book book = new Book("The Great Book");
Magazine mag = new Magazine("Tech Monthly");

printItem<Book>(book);
printItem<Magazine>(mag);
