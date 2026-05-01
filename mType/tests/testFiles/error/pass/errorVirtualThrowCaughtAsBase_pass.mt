// Test: a virtual method overridden in two subclasses throws a
// derived exception type from each; the catch declared at the base
// exception type catches both via polymorphism.
import * from "../../lib/exceptions/Exception.mt";

class WorkException extends Exception {
    public constructor(string m) : super(m) {}
}

class CriticalWorkException extends WorkException {
    public constructor(string m) : super(m) {}
}

abstract class Worker {
    abstract function doIt(): void;
}

class SafeWorker extends Worker {
    public function doIt(): void {
        throw new WorkException("safe");
    }
}

class CriticalWorker extends Worker {
    public function doIt(): void {
        throw new CriticalWorkException("critical");
    }
}

function run(Worker w): void {
    try {
        w.doIt();
    } catch (WorkException e) {
        print("caught WorkException: " + e.getMessage());
    }
}

function main(): void {
    run(new SafeWorker());
    run(new CriticalWorker());
}
main();
