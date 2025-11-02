import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test: Recursive type bounds
// This tests the F-bounded polymorphism pattern (Curiously Recurring Template Pattern)

interface Comparable<T> {
    public function compareTo(T other): int;
}

// Recursive bound: T extends ComparableEntity<T>
// This means T must extend ComparableEntity with itself as the type parameter
class ComparableEntity<T extends ComparableEntity<T>> implements Comparable<T> {
    int id;

    constructor(int entityId) {
        id = entityId;
    }

    public function compareTo(T other): int {
        if (id < other.getId()) {
            return -1;
        } else if (id > other.getId()) {
            return 1;
        }
        return 0;
    }

    public function getId(): int {
        return id;
    }
}

// User properly extends ComparableEntity with User as type parameter
class User extends ComparableEntity<User> {
    string name;

    constructor(int userId, string userName) {
        super(userId);
        name = userName;
    }

    public function getName(): string {
        return name;
    }

    public function describe(): string {
        return "User{id=" + id + ", name=" + name + "}";
    }
}

// Product properly extends ComparableEntity with Product as type parameter
class Product extends ComparableEntity<Product> {
    string productName;
    int price;

    constructor(int productId, string name, int productPrice) {
        super(productId);
        productName = name;
        price = productPrice;
    }

    public function getProductName(): string {
        return productName;
    }

    public function getPrice(): int {
        return price;
    }

    public function describe(): string {
        return "Product{id=" + id + ", name=" + productName + ", price=" + price + "}";
    }
}

// Generic function that works with recursive bounds
function <T extends ComparableEntity<T>> findMax(T first, T second): T {
    int comparison = first.compareTo(second);
    if (comparison >= 0) {
        return first;
    }
    return second;
}

function main(): void {
    print("Testing recursive type bounds (F-bounded polymorphism)");

    User user1 = new User(1, "Alice");
    User user2 = new User(2, "Bob");
    User user3 = new User(3, "Charlie");

    print(user1.describe());
    print(user2.describe());
    print(user3.describe());

    User maxUser = findMax<User>(user1, user2);
    print("Max user between user1 and user2: " + maxUser.describe());

    maxUser = findMax<User>(user2, user3);
    print("Max user between user2 and user3: " + maxUser.describe());

    Product prod1 = new Product(100, "Laptop", 1200);
    Product prod2 = new Product(101, "Mouse", 25);
    Product prod3 = new Product(102, "Keyboard", 75);

    print(prod1.describe());
    print(prod2.describe());
    print(prod3.describe());

    Product maxProduct = findMax<Product>(prod1, prod2);
    print("Max product between prod1 and prod2: " + maxProduct.describe());

    maxProduct = findMax<Product>(prod2, prod3);
    print("Max product between prod2 and prod3: " + maxProduct.describe());

    int userComparison = user1.compareTo(user2);
    print("user1.compareTo(user2) = " + userComparison);

    int productComparison = prod2.compareTo(prod3);
    print("prod2.compareTo(prod3) = " + productComparison);

    print("Recursive type bounds test completed successfully");
}

main();
