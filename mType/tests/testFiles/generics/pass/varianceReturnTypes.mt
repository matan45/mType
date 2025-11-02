import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Variance in return types (covariant)
class Product {
    String name;

    public function Product(String n) {
        name = n;
    }

    public function getName(): String {
        return name;
    }
}

class Book extends Product {
    public function Book(String n) {
        super(n);
    }
}

class Factory<T> {
    function(): T creator;

    public function setCreator(function(): T c): void {
        creator = c;
    }

    public function create(): T {
        return creator();
    }
}

function main(): void {
    Factory<Book> bookFactory = new Factory<Book>();
    bookFactory.setCreator(function(): Book {
        return new Book(new String("Novel"));
    });

    // Covariant return - can treat Book factory result as Product
    Book book = bookFactory.create();
    Product product = book;
    print("Product: " + product.getName());
}

main();
