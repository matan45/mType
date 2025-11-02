// Test: Realistic scenario combining everything
// @Script

import "modules/ServiceInterfaces.mt";

class User {
    private id: Int;
    private username: String;
    private email: String;

    constructor(id: Int, username: String, email: String) {
        this.id = id;
        this.username = username;
        this.email = email;
    }

    getId() : Int {
        return this.id;
    }

    getUsername() : String {
        return this.username;
    }

    getEmail() : String {
        return this.email;
    }
}

class UserServiceImpl : AsyncUserService, AsyncLogger {
    private users: String[];

    constructor() {
        this.users = [];
    }

    async authenticate(username: String, password: String) : Promise<Bool> {
        await this.logInfo("Authentication attempt: " + username);
        await delay(10);
        // Simplified authentication
        return username.length() > 0 && password.length() > 0;
    }

    async getUserData(userId: Int) : Promise<String> {
        await this.logInfo("Fetching user data: " + userId.toString());
        await delay(10);
        if (userId >= 0 && userId < this.users.length()) {
            return this.users[userId];
        }
        return "Unknown user";
    }

    async logInfo(message: String) : Promise<Void> {
        await delay(5);
        print("[INFO] " + message);
    }

    async logError(message: String) : Promise<Void> {
        await delay(5);
        print("[ERROR] " + message);
    }

    addUser(userData: String) : Void {
        this.users.push(userData);
    }
}

class DataServiceImpl<T> : AsyncDataService<T>, AsyncLogger {
    private storage: T[];
    private ids: Int[];

    constructor() {
        this.storage = [];
        this.ids = [];
    }

    async load(id: Int) : Promise<T> {
        await this.logInfo("Loading data with ID: " + id.toString());
        await delay(10);
        let i: Int = 0;
        while (i < this.ids.length()) {
            if (this.ids[i] == id) {
                return this.storage[i];
            }
            i = i + 1;
        }
        return this.storage[0]; // Default
    }

    async save(id: Int, data: T) : Promise<Bool> {
        await this.logInfo("Saving data with ID: " + id.toString());
        await delay(10);
        this.ids.push(id);
        this.storage.push(data);
        return true;
    }

    async query(filter: (T) -> Bool) : Promise<T[]> {
        await this.logInfo("Querying data");
        await delay(10);
        let results: T[] = [];
        let i: Int = 0;
        while (i < this.storage.length()) {
            if (filter(this.storage[i])) {
                results.push(this.storage[i]);
            }
            i = i + 1;
        }
        return results;
    }

    async logInfo(message: String) : Promise<Void> {
        await delay(5);
        print("[DATA-INFO] " + message);
    }

    async logError(message: String) : Promise<Void> {
        await delay(5);
        print("[DATA-ERROR] " + message);
    }
}

async processUserWorkflow(
    userService: AsyncUserService,
    dataService: AsyncDataService<User>,
    username: String,
    password: String
) : Promise<Bool> {
    // Authenticate
    let authenticated = await userService.authenticate(username, password);
    if (!authenticated) {
        return false;
    }

    // Load user data
    let userData = await userService.getUserData(0);
    print("User data: " + userData);

    // Query with lambda
    let activeUsers = await dataService.query((u: User) : Bool => {
        return u.getUsername().length() > 0;
    });

    print("Active users count: " + activeUsers.length().toString());
    return true;
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let userService = new UserServiceImpl();
    userService.addUser("alice@example.com");
    userService.addUser("bob@example.com");

    let dataService = new DataServiceImpl<User>();

    // Save users
    let user1 = new User(1, "alice", "alice@example.com");
    let user2 = new User(2, "bob", "bob@example.com");

    let saved1 = await dataService.save(1, user1);
    let saved2 = await dataService.save(2, user2);

    assert(saved1 && saved2, "Should save users");

    // Load user
    let loaded = await dataService.load(1);
    print("Loaded user: " + loaded.getUsername());
    assert(loaded.getUsername() == "alice", "Should load correct user");

    // Process workflow
    let success = await processUserWorkflow(
        userService,
        dataService,
        "alice",
        "password123"
    );
    assert(success, "Workflow should complete successfully");

    // Query with complex lambda
    let emailQuery = await dataService.query((u: User) : Bool => {
        return u.getEmail().length() > 10;
    });
    print("Users with long emails: " + emailQuery.length().toString());
    assert(emailQuery.length() == 2, "Should find users with long emails");

    print("Real-world scenario test passed");
}
