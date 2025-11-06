// Test: Realistic scenario combining everything
// @Script

import "modules/ServiceInterfaces.mt";

class User {
    private Int id;
    private String username;
    private String email;

    constructor(Int id, String username, String email) {
        this.id = id;
        this.username = username;
        this.email = email;
    }

    function getId() : Int {
        return this.id;
    }

    function getUsername() : String {
        return this.username;
    }

    function getEmail() : String {
        return this.email;
    }
}

class UserServiceImpl implements AsyncUserService, AsyncLogger {
    private String[] users;

    constructor() {
        this.users = [];
    }

    function async authenticate(String username, String password) : Promise<Bool> {
        await this.logInfo("Authentication attempt: " + username);
        await delay(10);
        // Simplified authentication
        return username.length() > 0 && password.length() > 0;
    }

    function async getUserData(Int userId) : Promise<String> {
        await this.logInfo("Fetching user data: " + userId.toString());
        await delay(10);
        if (userId >= 0 && userId < this.users.length()) {
            return this.users[userId];
        }
        return "Unknown user";
    }

    function async logInfo(String message) : Promise<void> {
        await delay(5);
        print("[INFO] " + message);
    }

    function async logError(String message) : Promise<void> {
        await delay(5);
        print("[ERROR] " + message);
    }

    function addUser(String userData) : void {
        this.users.push(userData);
    }
}

class DataServiceImpl<T> implements AsyncDataService<T>, AsyncLogger {
    private T[] storage;
    private Int[] ids;

    constructor() {
        this.storage = [];
        this.ids = [];
    }

    function async load(Int id) : Promise<T> {
        await this.logInfo("Loading data with ID: " + id.toString());
        await delay(10);
        Int i = 0;
        while (i < this.ids.length()) {
            if (this.ids[i] == id) {
                return this.storage[i];
            }
            i = i + 1;
        }
        return this.storage[0]; // Default
    }

    function async save(Int id, T data) : Promise<Bool> {
        await this.logInfo("Saving data with ID: " + id.toString());
        await delay(10);
        this.ids.push(id);
        this.storage.push(data);
        return true;
    }

    function async query((T) -> Bool filter) : Promise<T[]> {
        await this.logInfo("Querying data");
        await delay(10);
        T[] results = [];
        Int i = 0;
        while (i < this.storage.length()) {
            if (filter(this.storage[i])) {
                results.push(this.storage[i]);
            }
            i = i + 1;
        }
        return results;
    }

    function async logInfo(String message) : Promise<void> {
        await delay(5);
        print("[DATA-INFO] " + message);
    }

    function async logError(String message) : Promise<void> {
        await delay(5);
        print("[DATA-ERROR] " + message);
    }
}

function async processUserWorkflow(
    AsyncUserService userService,
    AsyncDataService<User> dataService,
    String username,
    String password
) : Promise<Bool> {
    // Authenticate
    Bool authenticated = await userService.authenticate(username, password);
    if (!authenticated) {
        return false;
    }

    // Load user data
    String userData = await userService.getUserData(0);
    print("User data: " + userData);

    // Query with lambda
    User[] activeUsers = await dataService.query((User u) : Bool => {
        return u.getUsername().length() > 0;
    });

    print("Active users count: " + activeUsers.length().toString());
    return true;
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    UserServiceImpl userService = new UserServiceImpl();
    userService.addUser("alice@example.com");
    userService.addUser("bob@example.com");

    DataServiceImpl<User> dataService = new DataServiceImpl<User>();

    // Save users
    User user1 = new User(1, "alice", "alice@example.com");
    User user2 = new User(2, "bob", "bob@example.com");

    Bool saved1 = await dataService.save(1, user1);
    Bool saved2 = await dataService.save(2, user2);

    assert(saved1 && saved2, "Should save users");

    // Load user
    User loaded = await dataService.load(1);
    print("Loaded user: " + loaded.getUsername());
    assert(loaded.getUsername() == "alice", "Should load correct user");

    // Process workflow
    Bool success = await processUserWorkflow(
        userService,
        dataService,
        "alice",
        "password123"
    );
    assert(success, "Workflow should complete successfully");

    // Query with complex lambda
    User[] emailQuery = await dataService.query((User u) : Bool => {
        return u.getEmail().length() > 10;
    });
    print("Users with long emails: " + emailQuery.length().toString());
    assert(emailQuery.length() == 2, "Should find users with long emails");

    print("Real-world scenario test passed");
}
