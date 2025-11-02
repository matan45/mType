@Script
// Test covariant return type casting
class Product {
    fn getName(): String {
        return "Generic Product";
    }
}

class Book : Product {
    fn getName(): String {
        return "Book";
    }

    fn getPages(): Int {
        return 300;
    }
}

class Electronics : Product {
    fn getName(): String {
        return "Electronics";
    }

    fn getWarranty(): Int {
        return 2;
    }
}

class Factory {
    fn createProduct(): Product {
        return new Product();
    }
}

class BookFactory : Factory {
    fn createProduct(): Product {
        // Return more specific type, cast to base
        return new Book() as Product;
    }
}

class ElectronicsFactory : Factory {
    fn createProduct(): Product {
        return new Electronics() as Product;
    }
}

fn main() {
    let bookFactory: BookFactory = new BookFactory();
    let product: Product = bookFactory.createProduct();
    print(product.getName());  // Should be "Book" due to polymorphism

    // Downcast to get specific functionality
    let book: Book = product as Book;
    if (book != null) {
        print(book.getPages());
    }

    let elecFactory: ElectronicsFactory = new ElectronicsFactory();
    let elecProduct: Product = elecFactory.createProduct();
    print(elecProduct.getName());

    let electronics: Electronics = elecProduct as Electronics;
    if (electronics != null) {
        print(electronics.getWarranty());
    }
}
