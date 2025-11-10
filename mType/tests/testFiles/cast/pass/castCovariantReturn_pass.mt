
// Test covariant return type casting
class Product {
    public function getName(): string {
        return "Generic Product";
    }
}

class Book extends Product {
    public function getName(): string {
        return "Book";
    }

    public function getPages(): int {
        return 300;
    }
}

class Electronics extends Product {
    public function getName(): string {
        return "Electronics";
    }

    public function getWarranty(): int {
        return 2;
    }
}

class Factory {
    public function createProduct(): Product {
        return new Product();
    }
}

class BookFactory extends Factory {
    public function createProduct(): Product {
        // Return more specific type, cast to base
        return new Book();
    }
}

class ElectronicsFactory extends Factory {
    public function createProduct(): Product {
        return new Electronics();
    }
}

function main(): void {
    BookFactory bookFactory = new BookFactory();
    Product product = bookFactory.createProduct();
    print(product.getName());  // Should be "Book" due to polymorphism

    // Downcast to get specific functionality
    Book book = (Book)product;
    if (book != null) {
        print(book.getPages());
    }

    ElectronicsFactory elecFactory = new ElectronicsFactory();
    Product elecProduct = elecFactory.createProduct();
    print(elecProduct.getName());

    Electronics electronics = (Electronics)elecProduct;
    if (electronics != null) {
        print(electronics.getWarranty());
    }
}

main();
