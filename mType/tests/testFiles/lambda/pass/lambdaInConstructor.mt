// Creating lambda in constructor test
interface Function {
    function apply(int x) : int;
}

class LambdaHolder {
    Function func;
    int baseValue;

    function init(int base) {
        this.baseValue = base;
        // Create lambda in constructor
        this.func = x -> {
            return this.baseValue + x;
        };
    }

    function execute(int x) : int {
        return this.func.apply(x);
    }

    function setBase(int newBase) {
        this.baseValue = newBase;
    }
}

class MultiLambdaHolder {
    Function adder;
    Function multiplier;
    int value;

    function init(int val) {
        this.value = val;
        // Multiple lambdas in constructor
        this.adder = x -> this.value + x;
        this.multiplier = x -> this.value * x;
    }

    function add(int x) : int {
        return this.adder.apply(x);
    }

    function multiply(int x) : int {
        return this.multiplier.apply(x);
    }
}

print("=== Lambda In Constructor Test ===");

LambdaHolder h1 = new LambdaHolder(100);
print("h1.execute(10): " + h1.execute(10));
print("h1.execute(20): " + h1.execute(20));

h1.setBase(200);
print("After setBase(200):");
print("h1.execute(10): " + h1.execute(10));

MultiLambdaHolder mh = new MultiLambdaHolder(5);
print("mh.add(10): " + mh.add(10));
print("mh.multiply(10): " + mh.multiply(10));

print("Lambda in constructor complete");
