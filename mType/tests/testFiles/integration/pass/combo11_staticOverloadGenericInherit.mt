// Combo 11: Static Methods + Overloading + Generics + Inheritance

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

abstract class Serializer {
    public static function serialize(int value): string {
        return "int:" + value;
    }

    public static function serialize(string value): string {
        return "str:" + value;
    }

    public static function serialize(float value): string {
        return "float:" + value;
    }

    public static function serialize(bool value): string {
        return "bool:" + value;
    }

    public static function <T> wrap(T value, string tag): string {
        return "<" + tag + ">" + value + "</" + tag + ">";
    }

    abstract function getName(): string;
}

class JsonSerializer extends Serializer {
    public function getName(): string {
        return "JSON";
    }

    public static function serializeJson(int value): string {
        return "{\"value\":" + value + "}";
    }

    public static function serializeJson(string value): string {
        return "{\"value\":\"" + value + "\"}";
    }
}

class XmlSerializer extends Serializer {
    public function getName(): string {
        return "XML";
    }

    public static function serializeXml(int value): string {
        return "<value>" + value + "</value>";
    }

    public static function serializeXml(string value): string {
        return "<value>" + value + "</value>";
    }
}

// Generic utility with static methods using boxed types
class CollectionUtils {
    public static function <T> first(T[] items): T {
        return items[0];
    }

    public static function <T> contains(T[] items, T target): bool {
        for (int i = 0; i < items.length; i++) {
            if (items[i] == target) {
                return true;
            }
        }
        return false;
    }
}

function main(): void {
    print("=== Combo 11: Static + Overload + Generics + Inheritance ===");

    // Static overloaded methods
    print("--- Base serializer ---");
    print(Serializer::serialize(42));
    print(Serializer::serialize("hello"));
    print(Serializer::serialize(3.14));
    print(Serializer::serialize(true));

    // Generic static method
    print("--- Generic wrap ---");
    print(Serializer::wrap("content", "div"));
    print(Serializer::wrap(42, "num"));

    // Subclass static methods
    print("--- JSON serializer ---");
    print(JsonSerializer::serializeJson(100));
    print(JsonSerializer::serializeJson("test"));

    print("--- XML serializer ---");
    print(XmlSerializer::serializeXml(200));
    print(XmlSerializer::serializeXml("data"));

    // Polymorphism
    print("--- Polymorphic getName ---");
    Serializer s1 = new JsonSerializer();
    Serializer s2 = new XmlSerializer();
    print(s1.getName());
    print(s2.getName());

    // Generic static utility with boxed Int[]
    print("--- CollectionUtils ---");
    Int[] nums = [new Int(10), new Int(20), new Int(30), new Int(40), new Int(50)];
    Int f = CollectionUtils::first(nums);
    print("First: " + f.getValue());

    // String array with boxed String[]
    String[] names = [new String("Alice"), new String("Bob"), new String("Charlie")];
    String firstName = CollectionUtils::first(names);
    print("First name: " + firstName.getValue());

    print("=== Combo 11 Complete ===");
}

main();
