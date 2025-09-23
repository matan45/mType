import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

class SimpleFactory {
    // Return the same object passed in
    static function <T> echo(T item): T {
        return item;
    }

    // Return null for any type
    static function <T> createNull(): T {
        return null;
    }

    // Work with multiple parameters and return first one
    static function <T, U> selectFirst(T first, U second): T {
        return first;
    }

    // Work with multiple parameters and return second one
    static function <T, U> selectSecond(T first, U second): U {
        return second;
    }
}

// Test echo functionality
String echoString = SimpleFactory::echo<String>(new String("hello"));
print("Echo string: " + echoString);

Int echoInt = SimpleFactory::echo<Int>(new Int(42));
print("Echo int: " + echoInt);

// Test null returns
String nullString = SimpleFactory::createNull<String>();
print("Null string: " + nullString);

Int nullInt = SimpleFactory::createNull<Int>();
print("Null int: " + nullInt);

// Test multiple type parameters
String firstResult = SimpleFactory::selectFirst<String, Int>(new String("first"), new Int(123));
print("Select first: " + firstResult);

Int secondResult = SimpleFactory::selectSecond<String, Int>(new String("test"), new Int(456));
print("Select second: " + secondResult);

print("Static generic simple return tests passed");