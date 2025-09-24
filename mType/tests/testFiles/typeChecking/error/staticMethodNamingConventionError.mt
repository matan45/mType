// Test: Static method names must start with lowercase letter
// Expected Error: Static method 'CreateInstance' must start with a lowercase letter. Static methods should follow camelCase convention.

class Utils {
    static function CreateInstance(): Utils {
        return new Utils();
    }

    constructor() {
        // Empty constructor
    }
}

function main(): void {
    Utils u = Utils.CreateInstance();
    print("Created instance");
}

main();