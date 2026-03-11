// Test: Invoking a private method without setAccessible(true) should throw error

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class Secret {
    private function hidden(): int {
        return 42;
    }
}

Class secretClass = Class::forName("Secret");
Method hiddenMethod = secretClass.getDeclaredMethod("hidden", 0);

Secret obj = new Secret();
int[] emptyArgs = new int[0];

// accessible is false by default — this should throw RuntimeException
int result = __reflect_invokeMethod(obj, hiddenMethod.getNativeHandle(), emptyArgs, hiddenMethod.isAccessible());

print("Should not reach here");
