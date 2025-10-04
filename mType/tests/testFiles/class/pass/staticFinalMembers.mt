class Constants {
    public static final int MAX_SIZE = 100;
    public static final float PI = 3.14159;
    public static final string VERSION = "1.0.0";

    public static function getMaxSize(): int {
        return MAX_SIZE;
    }

    public static function getPi(): float {
        return PI;
    }

    public static function getVersion(): string {
        return VERSION;
    }
}

print(Constants::MAX_SIZE); // 100
print(Constants::PI); // 3.14159
print(Constants::VERSION); // 1.0.0

print(Constants::getMaxSize()); // 100
print(Constants::getPi()); // 3.14159
print(Constants::getVersion()); // 1.0.0

// This should error if uncommented:
// Constants::MAX_SIZE = 200; // Error: Cannot modify final variable