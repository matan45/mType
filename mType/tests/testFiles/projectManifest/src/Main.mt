// Main entry point
import * from "core/Calculator.mt";
import * from "utils/Helper.mt";

class Main {
    public static function main(): void {
        Calculator calc = new Calculator();
        int result = calc.add(5, 3);
        print(result);

        // Test static method call
        string formatted = Helper::formatNumber(result);
        print(formatted);

        bool positive = Helper::isPositive(result);
        print(positive);
    }
}

Main::main();
