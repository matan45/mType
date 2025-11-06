// Test: Polymorphic casting with exception handling integration
// @Script

interface Serializable {
    function serialize() : String;
}

interface Validatable {
    function validate() : Bool;
}

class BaseEntity implements Serializable {
    protected Int id;
    protected String name;

    constructor(Int i, String n) {
        this.id = i;
        this.name = n;
    }

    function serialize() : String {
        return "BaseEntity(" + this.id.toString() + ", " + this.name + ")";
    }

    function getId() : Int {
        return this.id;
    }

    function getName() : String {
        return this.name;
    }
}

class User extends BaseEntity implements Validatable {
    private String email;
    private Bool isActive;

    constructor(Int i, String n, String e, Bool active) {
        super(i, n);
        this.email = e;
        this.isActive = active;
    }

    function serialize() : String {
        return "User(" + this.id.toString() + ", " + this.name + ", " + this.email + ")";
    }

    function validate() : Bool {
        if (this.email.length() == 0) {
            throw "Invalid email: empty";
        }
        if (!this.isActive) {
            throw "User is inactive";
        }
        return true;
    }

    function getEmail() : String {
        return this.email;
    }

    function setActive(Bool active) : void {
        this.isActive = active;
    }
}

class Admin extends User {
    private String[] permissions;

    constructor(Int i, String n, String e, Bool active, String[] perms) {
        super(i, n, e, active);
        this.permissions = perms;
    }

    function serialize() : String {
        String permStr = "";
        Int idx = 0;
        while (idx < this.permissions.length()) {
            if (idx > 0) {
                permStr = permStr + ",";
            }
            permStr = permStr + this.permissions[idx];
            idx = idx + 1;
        }
        return "Admin(" + this.id.toString() + ", " + this.name + ", [" + permStr + "])";
    }

    function validate() : Bool {
        // Call parent validation
        super.validate();

        if (this.permissions.length() == 0) {
            throw "Admin must have permissions";
        }
        return true;
    }

    function hasPermission(String perm) : Bool {
        Int i = 0;
        while (i < this.permissions.length()) {
            if (this.permissions[i] == perm) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }
}

function processEntity<T extends BaseEntity>(T entity) : String {
    try {
        // Try to cast to Validatable
        Validatable? validatable = entity as Validatable;
        if (validatable != null) {
            try {
                if (validatable.validate()) {
                    print("Validation passed for: " + entity.getName());
                }
            } catch (String e) {
                print("Validation error: " + e);
                return "INVALID";
            }
        }

        // Serialize the entity
        Serializable serializable = entity as Serializable;
        String result = serializable.serialize();
        print("Serialized: " + result);
        return result;

    } catch (String e) {
        print("Processing error: " + e);
        return "ERROR";
    }
}

function handleMultipleEntities(BaseEntity[] entities) : Int {
    Int successCount = 0;
    Int idx = 0;

    while (idx < entities.length()) {
        try {
            BaseEntity entity = entities[idx];

            // Use polymorphic casting to determine type
            User? user = entity as User;
            if (user != null) {
                print("Processing user: " + user.getName());

                // Check if it's an admin
                Admin? admin = user as Admin;
                if (admin != null) {
                    print("User is admin with permissions");
                    if (admin.hasPermission("write")) {
                        print("Admin has write permission");
                    }
                }
            }

            String result = processEntity<BaseEntity>(entity);
            if (result != "INVALID" && result != "ERROR") {
                successCount = successCount + 1;
            }

        } catch (String e) {
            print("Error processing entity at index " + idx.toString() + ": " + e);
        }

        idx = idx + 1;
    }

    return successCount;
}

function main() : void {
    print("=== Polymorphic Exception Test ===");

    // Create test entities
    BaseEntity[] entities = new BaseEntity[4];

    // Valid base entity
    entities[0] = new BaseEntity(1, "Base1");

    // Valid user
    entities[1] = new User(2, "Alice", "alice@example.com", true);

    // Invalid user (empty email)
    entities[2] = new User(3, "Bob", "", true);

    // Valid admin
    String[] perms = new String[2];
    perms[0] = "read";
    perms[1] = "write";
    entities[3] = new Admin(4, "Charlie", "charlie@example.com", true, perms);

    // Process all entities
    Int count = handleMultipleEntities(entities);
    print("Successfully processed: " + count.toString() + " entities");

    // Test inactive user
    print("\n=== Test Inactive User ===");
    User inactiveUser = new User(5, "Dave", "dave@example.com", false);
    String result = processEntity<User>(inactiveUser);

    // Test admin without permissions
    print("\n=== Test Admin Without Permissions ===");
    String[] emptyPerms = new String[0];
    Admin adminNoPerms = new Admin(6, "Eve", "eve@example.com", true, emptyPerms);
    String result2 = processEntity<Admin>(adminNoPerms);

    print("\n=== All tests completed ===");
}
