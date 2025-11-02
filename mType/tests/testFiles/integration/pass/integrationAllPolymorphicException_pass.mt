// Test: Polymorphic casting with exception handling integration
// @Script

interface Serializable {
    serialize() : String;
}

interface Validatable {
    validate() : Bool;
}

class BaseEntity : Serializable {
    protected id: Int;
    protected name: String;

    constructor(i: Int, n: String) {
        this.id = i;
        this.name = n;
    }

    serialize() : String {
        return "BaseEntity(" + this.id.toString() + ", " + this.name + ")";
    }

    getId() : Int {
        return this.id;
    }

    getName() : String {
        return this.name;
    }
}

class User extends BaseEntity : Validatable {
    private email: String;
    private isActive: Bool;

    constructor(i: Int, n: String, e: String, active: Bool) {
        super(i, n);
        this.email = e;
        this.isActive = active;
    }

    serialize() : String {
        return "User(" + this.id.toString() + ", " + this.name + ", " + this.email + ")";
    }

    validate() : Bool {
        if (this.email.length() == 0) {
            throw "Invalid email: empty";
        }
        if (!this.isActive) {
            throw "User is inactive";
        }
        return true;
    }

    getEmail() : String {
        return this.email;
    }

    setActive(active: Bool) : Void {
        this.isActive = active;
    }
}

class Admin extends User {
    private permissions: String[];

    constructor(i: Int, n: String, e: String, active: Bool, perms: String[]) {
        super(i, n, e, active);
        this.permissions = perms;
    }

    serialize() : String {
        let permStr: String = "";
        let idx: Int = 0;
        while (idx < this.permissions.length()) {
            if (idx > 0) {
                permStr = permStr + ",";
            }
            permStr = permStr + this.permissions[idx];
            idx = idx + 1;
        }
        return "Admin(" + this.id.toString() + ", " + this.name + ", [" + permStr + "])";
    }

    validate() : Bool {
        // Call parent validation
        super.validate();

        if (this.permissions.length() == 0) {
            throw "Admin must have permissions";
        }
        return true;
    }

    hasPermission(perm: String) : Bool {
        let i: Int = 0;
        while (i < this.permissions.length()) {
            if (this.permissions[i] == perm) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }
}

processEntity<T extends BaseEntity>(entity: T) : String {
    try {
        // Try to cast to Validatable
        let validatable: Validatable? = entity as Validatable;
        if (validatable != null) {
            try {
                if (validatable.validate()) {
                    print("Validation passed for: " + entity.getName());
                }
            } catch (e: String) {
                print("Validation error: " + e);
                return "INVALID";
            }
        }

        // Serialize the entity
        let serializable: Serializable = entity as Serializable;
        let result: String = serializable.serialize();
        print("Serialized: " + result);
        return result;

    } catch (e: String) {
        print("Processing error: " + e);
        return "ERROR";
    }
}

handleMultipleEntities(entities: BaseEntity[]) : Int {
    let successCount: Int = 0;
    let idx: Int = 0;

    while (idx < entities.length()) {
        try {
            let entity: BaseEntity = entities[idx];

            // Use polymorphic casting to determine type
            let user: User? = entity as User;
            if (user != null) {
                print("Processing user: " + user.getName());

                // Check if it's an admin
                let admin: Admin? = user as Admin;
                if (admin != null) {
                    print("User is admin with permissions");
                    if (admin.hasPermission("write")) {
                        print("Admin has write permission");
                    }
                }
            }

            let result: String = processEntity<BaseEntity>(entity);
            if (result != "INVALID" && result != "ERROR") {
                successCount = successCount + 1;
            }

        } catch (e: String) {
            print("Error processing entity at index " + idx.toString() + ": " + e);
        }

        idx = idx + 1;
    }

    return successCount;
}

main() : Void {
    print("=== Polymorphic Exception Test ===");

    // Create test entities
    let entities: BaseEntity[] = new BaseEntity[4];

    // Valid base entity
    entities[0] = new BaseEntity(1, "Base1");

    // Valid user
    entities[1] = new User(2, "Alice", "alice@example.com", true);

    // Invalid user (empty email)
    entities[2] = new User(3, "Bob", "", true);

    // Valid admin
    let perms: String[] = new String[2];
    perms[0] = "read";
    perms[1] = "write";
    entities[3] = new Admin(4, "Charlie", "charlie@example.com", true, perms);

    // Process all entities
    let count: Int = handleMultipleEntities(entities);
    print("Successfully processed: " + count.toString() + " entities");

    // Test inactive user
    print("\n=== Test Inactive User ===");
    let inactiveUser: User = new User(5, "Dave", "dave@example.com", false);
    let result: String = processEntity<User>(inactiveUser);

    // Test admin without permissions
    print("\n=== Test Admin Without Permissions ===");
    let emptyPerms: String[] = new String[0];
    let adminNoPerms: Admin = new Admin(6, "Eve", "eve@example.com", true, emptyPerms);
    let result2: String = processEntity<Admin>(adminNoPerms);

    print("\n=== All tests completed ===");
}
