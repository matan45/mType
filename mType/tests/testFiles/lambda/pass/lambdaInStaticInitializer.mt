// Lambda in static initializer block test
interface Function {
    function apply(int x) : int;
}

class StaticLambdaExample {
    static Function staticFunc;

    public static int staticValue = 1000;
          
    static Function staticFunc = x -> staticValue + x;
    

    public function useStaticFunc(int x) : int {
        return staticFunc.apply(x);
    }

    public static function getStaticValue() : int {
        return staticValue;
    }
}

print("=== Lambda In Static Initializer Test ===");

print("Static value: " + StaticLambdaExample::getStaticValue());

StaticLambdaExample obj1 = new StaticLambdaExample();
print("obj1.useStaticFunc(50): " + obj1.useStaticFunc(50));

StaticLambdaExample obj2 = new StaticLambdaExample();
print("obj2.useStaticFunc(100): " + obj2.useStaticFunc(100));

// Modify static value
StaticLambdaExample::staticValue = 2000;
print("After changing static value to 2000:");
print("obj1.useStaticFunc(50): " + obj1.useStaticFunc(50));

print("Lambda in static initializer complete");
