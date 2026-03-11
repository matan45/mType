// Test: Import overloaded static methods with object parameter types
import { Wrapper, Processor } from "modules/ObjectParamOverloadModule.mt";

function main(): void {
    print(Processor::process(42));
    print(Processor::process("hello"));

    Wrapper w = new Wrapper();
    w.init(99);
    print(Processor::process(w));
}
main();
