// Test: Import generic interface and implement
// @Script

import "modules/GenericRepository.mt";

class User {
    private Int id;
    private String name;

    constructor(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }

    public function getId() : Int {
        return this.id;
    }

    public function getName() : String {
        return this.name;
    }
}

class UserRepository implements Repository<User> {
    private User[] users;
    private Int nextId;

    constructor() {
        this.users = [];
        this.nextId = 0;
    }

    public function save(item: User) : void {
        this.users.push(item);
        print("Saved user: " + item.getName());
    }

    public function findById(id: Int) : User {
        Int i = 0;
        while (i < this.users.length()) {
            if (this.users[i].getId() == id) {
                return this.users[i];
            }
            i = i + 1;
        }
        return this.users[0]; // Default
    }

    public function count() : Int {
        return this.users.length();
    }
}

function main() : void {
    Repository<User> repo = new UserRepository();
    User user1 = new User(1, "Alice");
    User user2 = new User(2, "Bob");

    repo.save(user1);
    repo.save(user2);

    Int count = repo.count();
    print("Total users: " + count.toString());
    assert(count == 2, "Should have 2 users");

    User found = repo.findById(1);
    assert(found.getName() == "Alice", "Should find Alice");
}
