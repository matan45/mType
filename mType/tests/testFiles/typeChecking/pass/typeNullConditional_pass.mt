// Test null-conditional operator type inference
// Validates that type checking works correctly with null checks

class Product {
    string name;
    int price;

    constructor(string n, int p) {
        name = n;
        price = p;
    }

    public function getName(): string {
        return name;
    }

    public function getPrice(): int {
        return price;
    }
}

class Order {
    Product product;

    constructor() {
        product = null;
    }

    public function setProduct(Product p): void {
        product = p;
    }

    public function getProduct(): Product {
        return product;
    }

    public function getProductName(): string {
        if (product != null) {
            return product.getName();
        } else {
            return "No product";
        }
    }

    public function getTotalPrice(): int {
        if (product != null) {
            return product.getPrice();
        } else {
            return 0;
        }
    }
}

function main(): void {
    print("Testing null-conditional type inference");

    Order order1 = new Order();

    // Type inference with null check
    string productName = order1.getProductName();
    print("Product name: " + productName);

    int totalPrice = order1.getTotalPrice();
    print("Total price: " + totalPrice);

    // Set product and check again
    order1.setProduct(new Product("Laptop", 1000));
    productName = order1.getProductName();
    print("Product name: " + productName);

    totalPrice = order1.getTotalPrice();
    print("Total price: " + totalPrice);

    // Conditional type narrowing
    Product p = order1.getProduct();
    if (p != null) {
        string name = p.getName();
        int price = p.getPrice();
        print("Product details: " + name + " - " + price);
    }

    print("Null-conditional type inference test completed");
}

main();
