// Test: a method called via reflection throws; the exception
// propagates through the reflection layer and a finally block fires
// after the catch handles it. Distinct from
// reflection/pass/reflectInvokeMethodThrowsCaughtByCaller.mt by
// asserting finally runs even when the reflective call threw.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/exceptions/Exception.mt";

class Worker {
    public constructor() { }
    public function run(): void {
        throw new Exception("worker boom");
    }
}

function main(): void {
    Class cls = Class::forName("Worker");
    Method m = cls.getDeclaredMethod("run", 0);
    Constructor ctor = cls.getDeclaredConstructor(0);
    Object[] noArgs = new Object[0];
    Object inst = ctor.newInstance(noArgs);

    bool finallyRan = false;
    try {
        m.invoke(inst, noArgs);
        print("FAIL no throw");
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    } finally {
        finallyRan = true;
        print("finally ran");
    }
    print("finallyRan=" + finallyRan);
}
main();
