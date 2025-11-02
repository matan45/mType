// Test interface returning generic collection
// @Script

interface Repository<T> {
    func findAll(): Array<T>;
    func findById(id: Int): T;
    func save(item: T): void;
}

class User {
    var id: Int;
    var name: String;

    func init(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }
}

class UserRepository implements Repository<User> {
    var users: Array<User>;

    func init() {
        this.users = new Array<User>();
    }

    func findAll(): Array<User> {
        return this.users;
    }

    func findById(id: Int): User {
        var i = 0;
        while (i < this.users.size()) {
            var user = this.users.get(i);
            if (user.id == id) {
                return user;
            }
            i = i + 1;
        }
        return null;
    }

    func save(item: User): void {
        this.users.add(item);
    }
}

var repo = new UserRepository();
repo.save(new User(1, "Alice"));
repo.save(new User(2, "Bob"));
repo.save(new User(3, "Charlie"));

var allUsers = repo.findAll();
print("All users:");
var i = 0;
while (i < allUsers.size()) {
    var user = allUsers.get(i);
    print("  " + user.id.toString() + ": " + user.name);
    i = i + 1;
}

var user = repo.findById(2);
if (user != null) {
    print("Found user: " + user.name);
}
