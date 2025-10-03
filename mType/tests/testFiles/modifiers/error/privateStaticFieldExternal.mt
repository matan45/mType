// Test: Private static field access violation from external context
class Database {
    private static string connectionString = "localhost:5432";

    public static string getConnection() {
        return connectionString;
    }
}

print(Database.connectionString);  // ERROR: Cannot access private static field
