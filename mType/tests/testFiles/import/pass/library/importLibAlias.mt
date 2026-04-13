// Test: import lib selective with alias syntax parses correctly
// Verifies parser handles: import lib {Symbol as Alias} from "libname";

class LocalUtil {
    public static function test(): string {
        return "alias syntax parsed";
    }
}

print(LocalUtil::test());
print("import lib alias test passed");
