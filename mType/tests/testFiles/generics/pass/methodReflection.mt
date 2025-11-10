import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method reflection-like behavior
class MethodInfo {
    String name;
    String returnType;

    public constructor(String n, String r) {
        name = n;
        returnType = r;
    }

    public function getName(): String {
        return name;
    }

    public function getReturnType(): String {
        return returnType;
    }
}

class Inspector {
    public static function <T> inspectMethod(String methodName, T sampleReturn): MethodInfo {
        return new MethodInfo(methodName, new String("Generic"));
    }
}

function main(): void {
    MethodInfo info1 = Inspector::inspectMethod<Int>(new String("process"), new Int(0));
    print("Method: " + info1.getName().toString());
    print("Return: " + info1.getReturnType().toString());

    MethodInfo info2 = Inspector::inspectMethod<String>(new String("convert"), new String(""));
    print("Method: " + info2.getName().toString());
    print("Return: " + info2.getReturnType().toString());
}

main();
