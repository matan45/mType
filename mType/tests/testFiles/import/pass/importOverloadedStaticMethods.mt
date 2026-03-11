// Test: Import overloaded static methods via selective import
import { MathUtils } from "modules/OverloadedStaticModule.mt";

function main(): void {
    int r1 = MathUtils::max(10, 20);
    print("max(10, 20) = " + r1);

    float r2 = MathUtils::max(3.5, 2.1);
    print("max(3.5, 2.1) = " + r2);

    int r3 = MathUtils::max(5, 15, 10);
    print("max(5, 15, 10) = " + r3);

    int r4 = MathUtils::abs(-42);
    print("abs(-42) = " + r4);
}
main();
