// Test: Import generic interface and implement
// @Script

import "modules/GenericRepository.mt";

class User {
    private id: Int;
    private name: String;

    constructor(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }

    getId() : Int {
        return this.id;
    }

    getName() : String {
        return this.name;
    }
}

class UserRepository : Repository<User> {
    private users: User[];
    private nextId: Int;

    constructor() {
        this.users = [];
        this.nextId = 0;
    }

    save(item: User) : Void {
        this.users.push(item);
        print("Saved user: " + item.getName());
    }

    findById(id: Int) : User {
        let i: Int = 0;
        while (i < this.users.length()) {
            if (this.users[i].getId() == id) {
                return this.users[i];
            }
            i = i + 1;
        }
        return this.users[0]; // Default
    }

    count() : Int {
        return this.users.length();
    }
}

main() : Void {
    let repo: Repository<User> = new UserRepository();
    let user1 = new User(1, "Alice");
    let user2 = new User(2, "Bob");

    repo.save(user1);
    repo.save(user2);

    let count = repo.count();
    print("Total users: " + count.toString());
    assert(count == 2, "Should have 2 users");

    let found = repo.findById(1);
    assert(found.getName() == "Alice", "Should find Alice");
}
