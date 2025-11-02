// Nested class lambda accessing enclosing private test
interface Function {
    function apply(int x) : int;
}

class Outer {
    int outerPrivate;

    function init(int val) {
        this.outerPrivate = val;
    }

    function createInnerLambda() : Function {
        int methodLocal = 50;

        // Lambda accessing outer private field and method local
        Function lambda = x -> {
            return this.outerPrivate + methodLocal + x;
        };

        return lambda;
    }

    function getPrivate() : int {
        return this.outerPrivate;
    }

    function setPrivate(int val) {
        this.outerPrivate = val;
    }
}

class ComplexOuter {
    int privateA;
    int privateB;

    function init(int a, int b) {
        this.privateA = a;
        this.privateB = b;
    }

    function createComplexLambda() : Function {
        Function lambda = x -> {
            // Lambda can access multiple private fields
            int sum = this.privateA + this.privateB;
            return sum * x;
        };
        return lambda;
    }
}

print("=== Access Enclosing Private Test ===");

Outer outer = new Outer(100);
Function lambda = outer.createInnerLambda();

print("Lambda(10): " + lambda.apply(10));
print("Lambda(20): " + lambda.apply(20));

outer.setPrivate(200);
Function lambda2 = outer.createInnerLambda();
print("After changing outer private:");
print("Lambda2(10): " + lambda2.apply(10));

ComplexOuter complex = new ComplexOuter(10, 20);
Function complexLambda = complex.createComplexLambda();
print("Complex(5): " + complexLambda.apply(5));
print("Complex(3): " + complexLambda.apply(3));

print("Access enclosing private complete");
