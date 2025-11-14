// Integration Test 15: Repository Pattern with Generics
// Tests: Generic repository + Collections + CRUD operations + Error handling

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/exceptions/Exception.mt";

// Entity base interface
interface Entity {
    function getId(): int;
    function setId(int id): void;
}

// Domain entities
class User implements Entity {
    private int id;
    private string username;
    private string email;

    constructor(string user, string mail) {
        this.id = 0;
        this.username = user;
        this.email = mail;
    }

    public function getId(): int {
        return this.id;
    }

    public function setId(int newId): void {
        this.id = newId;
    }

    public function getUsername(): string {
        return this.username;
    }

    public function getEmail(): string {
        return this.email;
    }

    public function toString(): string {
        return "User{id=" + this.id + ", username=" + this.username + ", email=" + this.email + "}";
    }
}

class Product implements Entity {
    private int id;
    private string name;
    private float price;

    constructor(string n, float p) {
        this.id = 0;
        this.name = n;
        this.price = p;
    }

    public function getId(): int {
        return this.id;
    }

    public function setId(int newId): void {
        this.id = newId;
    }

    public function getName(): string {
        return this.name;
    }

    public function getPrice(): float {
        return this.price;
    }

    public function toString(): string {
        return "Product{id=" + this.id + ", name=" + this.name + ", price=" + this.price + "}";
    }
}

// Repository exception
class RepositoryException extends Exception {

    constructor(string msg): super(msg) {
        
    }

    
}

// Generic repository interface
interface Repository<T extends Entity> {
    function save(T entity): T;
    function findById(int id): T;
    function findAll(): ArrayList<T>;
    function delete(int id): bool;
    function count(): int;
}

// Generic repository implementation
class InMemoryRepository<T extends Entity> implements Repository<T> {
    private HashMap<Int, T> storage;
    private int nextId;

    constructor() {
        this.storage = new HashMap<Int, T>();
        this.nextId = 1;
    }

    public function save(T entity): T {
        int entityId = entity.getId();

        if (entityId == 0) {
            // New entity - assign ID
            entity.setId(this.nextId);
            this.storage.put(new Int(this.nextId), entity);
            this.nextId = this.nextId + 1;
            print("Saved new entity with ID: " + entity.getId());
        } else {
            // Update existing
            this.storage.put(new Int(entityId), entity);
            print("Updated entity with ID: " + entityId);
        }

        return entity;
    }

    public function findById(int id): T {
        T result = this.storage.get(new Int(id));

        if (result == null) {
            print("Entity not found with ID: " + id);
        }

        return result;
    }

    public function findAll(): ArrayList<T> {
        // Get all values from HashMap
        ArrayList<T> result = new ArrayList<T>();
        Int[] keys = this.storage.getKeys();

        // Sort keys by ID for consistent ordering
        for (int i = 0; i < keys.length - 1; i = i + 1) {
            for (int j = 0; j < keys.length - i - 1; j = j + 1) {
                if (keys[j].getValue() > keys[j + 1].getValue()) {
                    Int temp = keys[j];
                    keys[j] = keys[j + 1];
                    keys[j + 1] = temp;
                }
            }
        }

        for (int i = 0; i < keys.length; i = i + 1) {
            T entity = this.storage.get(keys[i]);
            if (entity != null) {
                result.add(entity);
            }
        }

        return result;
    }

    public function delete(int id): bool {
        T removed = this.storage.remove(new Int(id));

        if (removed != null) {
            print("Deleted entity with ID: " + id);
            return true;
        }

        print("Cannot delete - entity not found: " + id);
        return false;
    }

    public function count(): int {
        return this.storage.size();
    }
}

// Service layer using repository
class UserService {
    private Repository<User> repository;

    constructor(Repository<User> repo) {
        this.repository = repo;
    }

    public function createUser(string username, string email): User {
        try {
            User user = new User(username, email);
            User saved = this.repository.save(user);
            print("UserService: Created user " + saved.getUsername());
            return saved;
        } catch (RepositoryException e) {
            print("Error creating user: " + e.getMessage());
            return null;
        }
    }

    public function getUserById(int id): User {
        User user = this.repository.findById(id);

        if (user != null) {
            print("UserService: Found " + user.getUsername());
        } else {
            print("UserService: User not found");
        }

        return user;
    }

    public function arrayListAllUsers(): void {
        ArrayList<User> users = this.repository.findAll();
        print("Total users: " + users.size());

        for (int i = 0; i < users.size(); i = i + 1) {
            User user = users.get(i);
            print("  " + user.toString());
        }
    }

    public function deleteUser(int id): bool {
        return this.repository.delete(id);
    }
}

// Main test execution
print("=== Test 15: Repository Pattern with Generics ===");

// Test 1: User repository CRUD operations
print("--- Test 1: User Repository ---");
InMemoryRepository<User> userRepo = new InMemoryRepository<User>();
UserService userService = new UserService(userRepo);

User u1 = userService.createUser("alice", "alice@example.com");
User u2 = userService.createUser("bob", "bob@example.com");
User u3 = userService.createUser("charlie", "charlie@example.com");

print("Repository count: " + userRepo.count());

// Test 2: Find by ID
print("--- Test 2: Find by ID ---");
User found = userService.getUserById(2);
if (found != null) {
    print("Found: " + found.toString());
}

User notFound = userService.getUserById(99);

// Test 3: ArrayList all
print("--- Test 3: ArrayList all users ---");
userService.arrayListAllUsers();

// Test 4: Delete
print("--- Test 4: Delete user ---");
bool deleted = userService.deleteUser(2);
print("Delete result: " + deleted);
print("Count after delete: " + userRepo.count());

userService.arrayListAllUsers();

// Test 5: Product repository (different entity type)
print("--- Test 5: Product Repository ---");
InMemoryRepository<Product> productRepo = new InMemoryRepository<Product>();

Product p1 = new Product("Laptop", 999.99);
Product p2 = new Product("Mouse", 29.99);
Product p3 = new Product("Keyboard", 79.99);

Product savedP1 = productRepo.save(p1);
Product savedP2 = productRepo.save(p2);
Product savedP3 = productRepo.save(p3);

print("Products count: " + productRepo.count());

ArrayList<Product> allProducts = productRepo.findAll();
print("All products:");
for (int i = 0; i < allProducts.size(); i = i + 1) {
    Product p = allProducts.get(i);
    print("  " + p.toString());
}

// Test 6: Update entity
print("--- Test 6: Update entity ---");
Product toUpdate = productRepo.findById(1);
if (toUpdate != null) {
    print("Before update: " + toUpdate.toString());
    // In a real scenario, you'd modify the entity
    productRepo.save(toUpdate);  // Save same entity (simulating update)
}

print("=== Test 15 Complete ===");
