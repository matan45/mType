// Test: Private constructor implementing singleton pattern
// Expected: Pass - demonstrates singleton pattern with private constructor

class Database {
    private static Database instance;
    private string connectionString;
    private int connectionCount;

    // Private constructor - cannot be called from outside
    private constructor() {
        this.connectionString = "localhost:5432";
        this.connectionCount = 0;
        print("Database instance created");
    }

    // Public static method to get the singleton instance
    public static function getInstance(): Database {
        if (instance == null) {
            instance = new Database();
        }
        return instance;
    }

    public function connect(): void {
        connectionCount++;
        print("Connected to " + this.connectionString + " (count: " + this.connectionCount + ")");
    }

    public function getConnectionCount(): int {
        return this.connectionCount;
    }
}

// Test singleton pattern
print("Getting first database instance:");
Database db1 = Database::getInstance();
db1.connect();

print("\nGetting second database instance:");
Database db2 = Database::getInstance();
db2.connect();

print("\nConnection count from db1: " + db1.getConnectionCount());
print("Connection count from db2: " + db2.getConnectionCount());
print("Same instance: " + (db1 == db2));
