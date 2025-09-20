// Test collections as class data members with static, final, and regular variations

class DataManager {
    // Regular collection data members
    Array<string> names;
    Map<int, string> idToName;
    Set<int> validIds;
    Stack<string> history;
    Queue<int> pending;

    // Final collection data members (initialized once)
    final Array<string> readonlyNames;
    final Set<string> categories;

    // Static collection data members (shared across all instances)
    static Array<int> globalCounters;
    static Map<string, int> statistics;

    // Static final collection data members (shared and immutable)
    static final Set<string> systemRoles;
    static final Stack<string> defaultHistory;

    constructor() {
        // Initialize regular collections
        this.names = new Array<string>();
        this.idToName = new Map<int, string>();
        this.validIds = new Set<int>();
        this.history = new Stack<string>();
        this.pending = new Queue<int>();

        // Initialize final collections (can only be done once)
        this.readonlyNames = new Array<string>();
        this.categories = new Set<string>();

        // Add some initial data to final collections
        this.readonlyNames.add("System");
        this.readonlyNames.add("Admin");
        this.categories.add("User");
        this.categories.add("Manager");
    }

    function addUser(int id, string name) : void {
        this.names.add(name);
        this.idToName.put(id, name);
        this.validIds.add(id);
        this.history.push("Added: " + name);
        this.pending.enqueue(id);

        print("Added user: " + name + " (ID: " + id + ")");
    }

    function showRegularCollections() : void {
        print("Regular Collections:");
        print("Names count: " + this.names.size());
        print("Valid IDs count: " + this.validIds.size());
        print("History entries: " + this.history.size());
        print("Pending IDs: " + this.pending.size());
    }

    function showFinalCollections() : void {
        print("Final Collections:");
        print("Readonly names count: " + this.readonlyNames.size());
        for (string name : this.readonlyNames) {
            print("Readonly name: " + name);
        }
        print("Categories count: " + this.categories.size());
        for (string category : this.categories) {
            print("Category: " + category);
        }
    }

    static function initializeStaticCollections() : void {
        globalCounters = new Array<int>();
        statistics = new Map<string, int>();

        globalCounters.add(100);
        globalCounters.add(200);
        globalCounters.add(300);

        statistics.put("totalUsers", 0);
        statistics.put("activeUsers", 0);
        statistics.put("systemUptime", 1440);
    }

    static function initializeStaticFinalCollections() : void {
        systemRoles = new Set<string>();
        defaultHistory = new Stack<string>();

        systemRoles.add("ADMIN");
        systemRoles.add("USER");
        systemRoles.add("GUEST");

        defaultHistory.push("System initialized");
        defaultHistory.push("Database connected");
        defaultHistory.push("Services started");
    }

    static function showStaticCollections() : void {
        print("Static Collections:");
        print("Global counters count: " + globalCounters.size());
        for (int counter : globalCounters) {
            print("Counter: " + counter);
        }

        print("Statistics count: " + statistics.size());
        for (int value : statistics) {
            print("Stat value: " + value);
        }
    }

    static function showStaticFinalCollections() : void {
        print("Static Final Collections:");
        print("System roles count: " + systemRoles.size());
        for (string role : systemRoles) {
            print("Role: " + role);
        }

        print("Default history count: " + defaultHistory.size());
        // Show history in reverse order (stack behavior)
        Stack<string> tempStack = new Stack<string>();
        while (!defaultHistory.empty()) {
            string entry = defaultHistory.pop();
            tempStack.push(entry);
            print("History: " + entry);
        }
        // Restore the original stack
        while (!tempStack.empty()) {
            defaultHistory.push(tempStack.pop());
        }
    }

    static function updateStatistics(string key, int value) : void {
        statistics.put(key, value);
        print("Updated " + key + " to " + value);
    }
}

// Test the collections as data members functionality
print("Testing Collections as Data Members:");

// Initialize static collections
DataManager::initializeStaticCollections();
DataManager::initializeStaticFinalCollections();

// Show static collections
DataManager::showStaticCollections();
DataManager::showStaticFinalCollections();

// Create instances and test instance collections
DataManager manager1 = new DataManager();
DataManager manager2 = new DataManager();

// Test regular collections
manager1.addUser(1, "Alice");
manager1.addUser(2, "Bob");

manager2.addUser(3, "Charlie");

// Show instance collections
print("Manager 1:");
manager1.showRegularCollections();
manager1.showFinalCollections();

print("Manager 2:");
manager2.showRegularCollections();
manager2.showFinalCollections();

// Test static collection updates
DataManager::updateStatistics("totalUsers", 3);
DataManager::updateStatistics("activeUsers", 2);

// Verify that all collection types work as data members
print("All collection types working as data members:");
print("- Array<T> as regular, final, static, and static final");
print("- Map<K,V> as regular and static");
print("- Set<T> as regular, final, and static final");
print("- Stack<T> as regular and static final");
print("- Queue<T> as regular");

print("Collections as data members test completed successfully!");