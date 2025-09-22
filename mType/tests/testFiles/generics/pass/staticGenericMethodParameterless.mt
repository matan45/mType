import "../../complie/lib/primitives/String.mt";
import "../../complie/lib/primitives/Int.mt";

class Factory {
    // Parameterless generic methods - useful for factory patterns
    static function <T> createDefault(): void {
        print("Creating default instance of type");
    }

    static function <T> getTypeName(): void {
        print("Getting type name");
    }

    // Multiple type parameters without using them in parameters
    static function <K, V> initializeMapping(): void {
        print("Initializing mapping for key-value types");
    }
}

// Test parameterless generic methods
Factory::createDefault<String>();
Factory::createDefault<Int>();

Factory::getTypeName<String>();
Factory::getTypeName<Int>();

// Test multiple type parameters without parameters
Factory::initializeMapping<String, Int>();
Factory::initializeMapping<Int, String>();

print("Parameterless static generic method tests passed");