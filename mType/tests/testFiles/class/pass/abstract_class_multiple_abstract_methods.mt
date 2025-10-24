// Test: Abstract class with multiple abstract methods
// Concrete classes must implement all of them

abstract class Database {
    abstract function connect(string host): bool;
    abstract function disconnect(): void;
    abstract function query(string sql): string;
    abstract function execute(string sql): int;

    // Concrete helper method
    public function isValidQuery(string sql): bool {
        return sql != "";
    }
}

class MySQLDatabase extends Database {
    private bool connected;

    constructor() {
        connected = false;
    }

    public function connect(string host): bool {
        print("Connecting to MySQL at " + host);
        connected = true;
        return true;
    }

    public function disconnect(): void {
        print("Disconnecting from MySQL");
        connected = false;
    }

    public function query(string sql): string {
        if (!connected) {
            return "ERROR: Not connected";
        }
        return "MySQL Result for: " + sql;
    }

    public function execute(string sql): int {
        if (!connected) {
            return -1;
        }
        print("Executing: " + sql);
        return 1;
    }
}

// Test the implementation
MySQLDatabase db = new MySQLDatabase();
db.connect("localhost");
print(db.query("SELECT * FROM users"));
int rows = db.execute("INSERT INTO users VALUES (1, 'John')");
print("Rows affected: " + rows);
db.disconnect();
