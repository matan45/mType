// Test interface returning generic collection
// @Script

import * from "../../lib/collections/ArrayList.mt";

interface Repository<T> {
    function findAll(): ArrayList<T>;
    function findById(int id): T?;
    function save(T item): void;
}

class User {
    public int id;
    public string name;

    public constructor(int id, string name) {
        this.id = id;
        this.name = name;
    }
}

class UserRepository implements Repository<User> {
    private ArrayList<User> users;

    public constructor() {
        this.users = new ArrayList<User>();
    }

    public function findAll(): ArrayList<User> {
        return this.users;
    }

    public function findById(int id): User? {
        for (int i = 0; i < this.users.size(); i++) {
            User user = this.users.get(i);
            if (user.id == id) {
                return user;
            }
        }
        return null;
    }

    public function save(User item): void {
        this.users.add(item);
    }
}

UserRepository repo = new UserRepository();
repo.save(new User(1, "Alice"));
repo.save(new User(2, "Bob"));
repo.save(new User(3, "Charlie"));

ArrayList<User> allUsers = repo.findAll();
print("All users:");
for (int i = 0; i < allUsers.size(); i++) {
    User user = allUsers.get(i);
    print("  " + user.id + ": " + user.name);
}

User user = repo.findById(2);
if (user != null) {
    print("Found user: " + user.name);
}
