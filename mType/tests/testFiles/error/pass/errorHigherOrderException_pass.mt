// Test: Higher-order functions returning functions with exception handling
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";

interface IntFunction {
    function apply(int value): int;
}

interface IntFunctionFactory {
    function create(int factor): IntFunction;
}

interface Validator {
    function validate(string input): bool;
}

interface ValidatorFactory {
    function createValidator(int minLength): Validator;
}

function main(): void {
    print("Testing higher-order functions with exceptions");

    // Factory that returns lambda with validation
    IntFunctionFactory multiplierFactory = factor -> {
        if (factor <= 0) {
            throw new Exception("Factory: factor must be positive");
        }

        // Return lambda that may also throw
        return x -> {
            if (x < 0) {
                throw new Exception("Multiplier: negative input");
            }
            return x * factor;
        };
    };

    // Create multiplier successfully
    try {
        IntFunction times5 = multiplierFactory.create(5);
        print("Created multiplier with factor 5");

        try {
            int result = times5.apply(10);
            print("Result: " + result);
        } catch (Exception e) {
            print("Application error: " + e.getMessage());
        }

        try {
            int result = times5.apply(-3);
            print("Result: " + result);
        } catch (Exception e) {
            print("Application error: " + e.getMessage());
        }
    } catch (Exception e) {
        print("Factory error: " + e.getMessage());
    }

    // Factory fails to create multiplier
    try {
        IntFunction invalidMultiplier = multiplierFactory.create(-2);
        print("Created multiplier");
    } catch (Exception e) {
        print("Factory error: " + e.getMessage());
    }

    // Validator factory with exception handling
    ValidatorFactory validatorFactory = minLen -> {
        if (minLen < 0) {
            throw new Exception("ValidatorFactory: minimum length cannot be negative");
        }

        return input -> {
            try {
                if (input == null) {
                    throw new Exception("Validator: null input");
                }
                if (input.length() < minLen) {
                    throw new Exception("Validator: input too short");
                }
                return true;
            } catch (Exception e) {
                print("Validation exception: " + e.getMessage());
                return false;
            }
        };
    };

    try {
        Validator lengthValidator = validatorFactory.createValidator(5);
        print("Created length validator");

        bool result1 = lengthValidator.validate("hello world");
        print("Validation result for 'hello world': " + result1);

        bool result2 = lengthValidator.validate("hi");
        print("Validation result for 'hi': " + result2);

        bool result3 = lengthValidator.validate(null);
        print("Validation result for null: " + result3);
    } catch (Exception e) {
        print("Validator factory error: " + e.getMessage());
    }

    // Chained higher-order function
    IntFunctionFactory composedFactory = baseFactor -> {
        if (baseFactor == 0) {
            throw new Exception("ComposedFactory: base factor cannot be zero");
        }

        return x -> {
            try {
                if (x == 0) {
                    throw new Exception("Composed: zero input");
                }
                int intermediate = x * baseFactor;
                if (intermediate > 10000) {
                    throw new Exception("Composed: result overflow");
                }
                return intermediate + baseFactor;
            } catch (Exception e) {
                print("Composed function error: " + e.getMessage());
                throw e;
            }
        };
    };

    try {
        IntFunction composed = composedFactory.create(10);

        try {
            int result = composed.apply(5);
            print("Composed result: " + result);
        } catch (Exception e) {
            print("Composed execution failed");
        }

        try {
            int result = composed.apply(0);
            print("Composed result: " + result);
        } catch (Exception e) {
            print("Composed execution failed for zero");
        }

        try {
            int result = composed.apply(1500);
            print("Composed result: " + result);
        } catch (Exception e) {
            print("Composed execution failed for overflow");
        }
    } catch (Exception e) {
        print("Composed factory error: " + e.getMessage());
    }

    print("Higher-order exception test completed");
}

main();
