// Test: toString() override for custom string representation
// Expected: Pass - demonstrates toString implementation

class Product {
    private string name;
    private float price;
    private int quantity;

    public constructor(string name, float price, int quantity) {
        this.name = name;
        this.price = price;
        this.quantity = quantity;
    }

    public function toString(): string {
        return "Product{name='" + this.name + "', price=" + this.price +
               ", quantity=" + this.quantity + "}";
    }
}

class Order {
    private int orderId;
    private Product[] items;
    private int itemCount;

    public constructor(int orderId) {
        this.orderId = orderId;
        this.items = new Product[10];
        this.itemCount = 0;
    }

    public function addItem(Product p): void {
        this.items[this.itemCount] = p;
        this.itemCount = this.itemCount + 1;
    }

    public function toString(): string {
        string result = "Order #" + this.orderId + " {\n";
        int i = 0;
        while (i < this.itemCount) {
            result = result + "  " + this.items[i].toString() + "\n";
            i = i + 1;
        }
        result = result + "}";
        return result;
    }
}

// Test toString override
print("Test 1: Product toString");
Product p1 = new Product("Laptop", 999.99, 5);
print(p1.toString());

print("\nTest 2: Multiple products");
Product p2 = new Product("Mouse", 29.99, 20);
Product p3 = new Product("Keyboard", 79.99, 15);
print(p2.toString());
print(p3.toString());

print("\nTest 3: Order with products");
Order order = new Order(12345);
order.addItem(p1);
order.addItem(p2);
order.addItem(p3);
print(order.toString());

print("\nTest 4: Direct print uses toString");
Product p4 = new Product("Monitor", 299.99, 8);
print("Direct print: " + p4);
