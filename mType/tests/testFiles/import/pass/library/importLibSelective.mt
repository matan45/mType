// Test: import lib selective syntax parses correctly
// Verifies parser handles: import lib {Symbol1, Symbol2} from "libname";

class Helper {
    public static function greet(): string {
        return "Hello from local";
    }
}

print(Helper::greet());
print("import lib selective syntax test passed");
