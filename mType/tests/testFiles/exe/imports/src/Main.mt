// Test: Imports in standalone exe (multi-file project)
import { MathUtils, StringUtils } from "Utils.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Use imported MathUtils
        int sum = MathUtils::add(10, 20);
        print("Add: " + sum);

        int product = MathUtils::multiply(6, 7);
        print("Multiply: " + product);

        int bigger = MathUtils::max(15, 25);
        print("Max: " + bigger);

        // Use imported StringUtils
        string repeated = StringUtils::repeat("ab", 3);
        print("Repeat: " + repeated);

        print("Imports test passed");
    }
}
