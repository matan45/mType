// Error: Cannot use await in static initializer

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Static Initializer Error Test ===");

function async getDefaultValue(): Promise<Int> {
    return new Int(100);
}

class Config {
    // ERROR: Cannot use await in static field initializer
    static int defaultValue = (await getDefaultValue()).getValue();

    public static function getDefault(): int {
        return Config.defaultValue;
    }
}

print("Default: " + Config.getDefault());
