// Test enhanced static method access error with location info
class Utils {
    static function helper(): int {
        return 42;
    }
}

Utils u = new Utils();
int result = u.helper();  // Line 8: Should show static method access error with location
print("This should not execute");