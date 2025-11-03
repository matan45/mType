// Test: Dynamic type verification at runtime
// Expected: Pass - demonstrates runtime type checking patterns

class Item {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public function getName(): string {
        return this.name;
    }
}

class Book extends Item {
    private string author;

    public constructor(string name, string author) : super(name) {
        this.author = author;
    }

    public function getAuthor(): string {
        return this.author;
    }
}

class DVD extends Item {
    private int duration;

    public constructor(string name, int duration) : super(name) {
        this.duration = duration;
    }

    public function getDuration(): int {
        return this.duration;
    }
}

class Processor {
    public function processItem(Item item): void {
        print("Processing: " + item.getName());

        if (item isClassOf Book) {
            Book book = (Book)item;
            print("  Type: Book");
            print("  Author: " + book.getAuthor());
        } else if (item isClassOf DVD) {
            DVD dvd = (DVD)item;
            print("  Type: DVD");
            print("  Duration: " + dvd.getDuration() + " minutes");
        } else {
            print("  Type: Generic Item");
        }
    }

    public function processArray(Item[] items): void {
        int i = 0;
        while (i < items.length) {
            this.processItem(items[i]);
            print("");
            i = i + 1;
        }
    }
}

// Test runtime type checking
Processor p = new Processor();

print("Test 1: Process individual items");
Item[] items = new Item[4];
items[0] = new Book("1984", "George Orwell");
items[1] = new DVD("Inception", 148);
items[2] = new Item("Generic Item");
items[3] = new Book("Dune", "Frank Herbert");

p.processArray(items);
