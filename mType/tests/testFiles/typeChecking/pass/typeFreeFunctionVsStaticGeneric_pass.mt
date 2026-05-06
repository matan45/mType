// Tier A: same generic signature reachable as a free function and as a
// static method. The two type-inference paths (inferFunctionCallType and
// inferMethodCallType) should both resolve the return type identically.
// Targets: TypeInferenceEngine.cpp:207 (function path - generic-poor)
//          vs TypeInferenceEngine.cpp:312-365 (method path - resolves R[]).
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Util {
    public static function <R> wrap(R value): R[] {
        R[] arr = new R[1];
        arr[0] = value;
        return arr;
    }
}

function <R> wrapFree(R value): R[] {
    R[] arr = new R[1];
    arr[0] = value;
    return arr;
}

// Method form — should be inferred as String[].
String[] viaStatic = Util::wrap<String>(new String("alpha"));
print(viaStatic[0]);

// Free function form — same R[] return signature.
String[] viaFree = wrapFree<String>(new String("beta"));
print(viaFree[0]);
