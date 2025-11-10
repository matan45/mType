// Test: Covariant return type in method override
// Expected: Pass - child method can return more specific type than parent

class Product {
    protected string name;
    protected float price;

    public constructor(string name, float price) {
        this.name = name;
        this.price = price;
    }

    public function clone():Product {
        return new Product(this.name, this.price);
    }

    public function getName(): string {
        return this.name;
    }

    public function getPrice(): float {
        return this.price;
    }

    public function display(): void {
        print("Product: " + this.name + ", Price: " + this.price);
    }
}

class Book extends Product {
    private string author;
    private int pages;

    public constructor(string name, float price, string author, int pages) : super(name, price) {
        this.author = author;
        this.pages = pages;
    }

    // Covariant return type: Book instead of Product
    public function clone(): Book {
        return new Book(this.name, this.price, this.author, this.pages);
    }

    public function getAuthor(): string {
        return this.author;
    }

    public function getPages(): int {
        return this.pages;
    }

    public function display(): void {
        print("Book: " + this.name + " by " + this.author + ", " + this.pages + " pages, Price: " + this.price);
    }
}

class Electronics extends Product {
    private string brand;
    private int warranty;

    public constructor(string name, float price, string brand, int warranty) : super(name, price) {
        this.brand = brand;
        this.warranty = warranty;
    }

    // Covariant return type: Electronics instead of Product
    public function clone() :Electronics{
        return new Electronics(this.name, this.price, this.brand, this.warranty);
    }

    public function getBrand(): string {
        return this.brand;
    }

    public function getWarranty(): int {
        return this.warranty;
    }

    public function display(): void {
        print("Electronics: " + this.name + " by " + this.brand + ", " + this.warranty + " year warranty, Price: " + this.price);
    }
}

// Test 1: Basic covariant return type
print("Test 1: Book covariant clone");
Book book1 = new Book("1984", 15.99, "George Orwell", 328);
book1.display();
Book book2 = book1.clone();  // Returns Book, not Product
book2.display();
print("Author of cloned book: " + book2.getAuthor());

// Test 2: Electronics covariant return type
print("\nTest 2: Electronics covariant clone");
Electronics device1 = new Electronics("Laptop", 999.99, "Dell", 2);
device1.display();
Electronics device2 = device1.clone();  // Returns Electronics, not Product
device2.display();
print("Warranty of cloned device: " + device2.getWarranty());

// Test 3: Polymorphic call with covariant return
print("\nTest 3: Polymorphic covariant return");
Product p1 = new Book("Dune", 18.99, "Frank Herbert", 688);
Product clonedProduct = p1.clone();  // Returns Product (polymorphic call)
clonedProduct.display();
print("Cloned product name: " + clonedProduct.getName());

// Test 4: Array of products with covariant cloning
print("\nTest 4: Array cloning");
Product[] products = new Product[2];
products[0] = new Book("Foundation", 14.99, "Isaac Asimov", 255);
products[1] = new Electronics("Tablet", 499.99, "Samsung", 1);

int i = 0;
while (i < 2) {
    print("Original:");
    products[i].display();
    Product cloned = products[i].clone();
    print("Clone:");
    cloned.display();
    i = i + 1;
}

// Test 5: Specific type cloning maintains type information
print("\nTest 5: Type-specific cloning");
Book specificBook = new Book("Neuromancer", 12.99, "William Gibson", 271);
Book clonedBook = specificBook.clone();  // Book type preserved
print("Cloned book pages: " + clonedBook.getPages());
print("Cloned book author: " + clonedBook.getAuthor());

Electronics specificDevice = new Electronics("Monitor", 299.99, "LG", 3);
Electronics clonedDevice = specificDevice.clone();  // Electronics type preserved
print("Cloned device brand: " + clonedDevice.getBrand());
print("Cloned device warranty: " + clonedDevice.getWarranty());

print("\nCovariant return type tests completed");
