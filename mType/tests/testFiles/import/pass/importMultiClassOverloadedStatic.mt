// Test: Import multiple classes with overloaded static methods
import { StringHelper, Counter } from "modules/MultiClassOverloadModule.mt";

function main(): void {
    string r1 = StringHelper::format("hello");
    print(r1);

    string r2 = StringHelper::format("foo", "bar");
    print(r2);

    int r3 = Counter::count(5);
    print("count = " + r3);
}
main();
