// Test method parameter introspection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class MathOps {
    public function noParams(): int {
        return 0;
    }

    public function oneParam(int x): int {
        return x;
    }

    public function twoParams(int x, int y): int {
        return x + y;
    }

    public function threeParams(string name, int age, bool active): string {
        return name;
    }
}

Class mathClass = Class::forName("MathOps");

// Test no parameters
Method noParams = mathClass.getDeclaredMethod("noParams", 0);
print("noParams param count: " + noParams.getParameterCount());

// Test one parameter
Method oneParam = mathClass.getDeclaredMethod("oneParam", 1);
print("oneParam param count: " + oneParam.getParameterCount());

// Test two parameters
Method twoParams = mathClass.getDeclaredMethod("twoParams", 2);
print("twoParams param count: " + twoParams.getParameterCount());

// Test three parameters
Method threeParams = mathClass.getDeclaredMethod("threeParams", 3);
print("threeParams param count: " + threeParams.getParameterCount());

// Get parameter types
string[] paramTypes = threeParams.getParameterTypes();
print("threeParams param types count: " + paramTypes.length);

print("Test passed");
