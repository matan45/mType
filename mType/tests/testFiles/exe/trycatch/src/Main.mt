// Test: Try/Catch in standalone exe
import * from "../../../lib/exceptions/Exception.mt";

class AppError extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

function riskyDivide(int a, int b): int {
    if (b == 0) {
        throw new AppError("Division by zero");
    }
    return a / b;
}

class Helper {
    public static function riskyStatic(int code): string {
        if (code == 0) {
            throw new AppError("Static error");
        }
        return "Static OK";
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Successful operation
        try {
            int result = riskyDivide(10, 2);
            print("10 / 2 = " + result);
        } catch (AppError e) {
            print("Error: " + e.getMessage());
        }

        // Catch thrown exception
        try {
            riskyDivide(10, 0);
        } catch (AppError e) {
            print("Caught: " + e.getMessage());
        }

        // Catch via base class
        try {
            throw new AppError("Caught as base");
        } catch (Exception e) {
            print("Base catch: " + e.getMessage());
        }

        // Nested try/catch
        try {
            try {
                throw new AppError("Inner error");
            } catch (AppError e) {
                print("Inner catch: " + e.getMessage());
                throw new AppError("Rethrown from inner");
            }
        } catch (AppError e) {
            print("Outer catch: " + e.getMessage());
        }

        // Catch from static method
        try {
            Helper::riskyStatic(0);
        } catch (AppError e) {
            print("Static catch: " + e.getMessage());
        }

        // Successful static call
        try {
            string result = Helper::riskyStatic(1);
            print("Static success: " + result);
        } catch (AppError e) {
            print("Unexpected");
        }

        // Code continues after catch
        print("After all catches");

        print("Try/Catch test passed");
    }
}
