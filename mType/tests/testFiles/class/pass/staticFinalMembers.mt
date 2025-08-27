class Constants {
    static final int MAX_SIZE = 100;
    static final float PI = 3.14159;
    static final string VERSION = "1.0.0";
    
    static function getMaxSize(): int {
        return MAX_SIZE;
    }
    
    static function getPi(): float {
        return PI;
    }
    
    static function getVersion(): string {
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