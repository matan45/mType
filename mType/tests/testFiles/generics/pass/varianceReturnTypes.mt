import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Variance in return types (covariant)
interface Creator<T> {
    function create(): T;
}

class Product {
    String name;

    public constructor(String n) {
        name = n;
    }

    public function getName(): String {
        return name;
    }
}

class Book extends Product {
    public constructor(String n) : super(n) {
    }
}

class Factory<T> {
    Creator<T> creator;

    public function setCreator(Creator<T> c): void {
        creator = c;
    }

    public function create(): T {
        return creator.create();
    }
}

function main(): void {
    Factory<Book> bookFactory = new Factory<Book>();
    bookFactory.setCreator(() -> new Book(new String("Novel")));

    // Covariant return - can treat Book factory result as Product
    Book book = bookFactory.create();
    Product product = book;
    print("Product: " + product.getName());
}

main();
