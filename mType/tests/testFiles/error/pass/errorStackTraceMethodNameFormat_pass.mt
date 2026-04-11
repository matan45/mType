// MYT-38: Lock in that method-call stack frames include both the class name and
// the method name. Per ExceptionExecutor.cpp:123 the frame name comes from
// CallFrame.functionName, which for instance methods is set to the qualifiedName
// (ClassName::methodName, e.g. via ClassMethodCallCompiler.cpp:32). This test
// pins both substrings — any future change to that format intentionally breaks it.
import * from "../../lib/exceptions/Exception.mt";

class Myt38Service {
    public function failingOperation(): void {
        throw new Exception("methodFormatTest");
    }
}

function main(): void {
    Myt38Service svc = new Myt38Service();
    try {
        svc.failingOperation();
    } catch (Exception e) {
        string trace = e.getStackTrace();

        int idxMethod = indexOf(trace, "failingOperation");
        int idxClass = indexOf(trace, "Myt38Service");
        int idxQualified = indexOf(trace, "Myt38Service::failingOperation");

        if (idxMethod != -1) { print("contains method name: true"); } else { print("contains method name: false"); }
        if (idxClass != -1) { print("contains class name: true"); } else { print("contains class name: false"); }
        if (idxQualified != -1) { print("contains qualified name: true"); } else { print("contains qualified name: false"); }
    }
}

main();
