import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test nested generic types
class Box<T> {
    T content;

    constructor(T c) {
        content = c;
    }

    public function getContent(): T {
        return content;
    }

    public function setContent(T newContent): void {
        content = newContent;
    }
}

class Wrapper<T> {
    T item;
    string label;

    constructor(T i, string l) {
        item = i;
        label = l;
    }

    public function getItem(): T {
        return item;
    }

    public function getLabel(): string {
        return label;
    }
}

// Nested generic instantiation
function main(): void {
    print("Testing nested generic instantiation");

    // Box containing Int
    Int num = new Int(42);
    Box<Int> simpleBox = new Box<Int>(num);
    print("Simple box: " + simpleBox.getContent());

    // Box containing another Box (nested generics)
    Box<Box<Int>> nestedBox = new Box<Box<Int>>(simpleBox);
    Box<Int> innerBox = nestedBox.getContent();
    print("Nested box inner: " + innerBox.getContent());

    // Wrapper containing Box
    String str = new String("Hello");
    Box<String> strBox = new Box<String>(str);
    Wrapper<Box<String>> wrappedBox = new Wrapper<Box<String>>(strBox, "StringBox");
    print("Wrapped box label: " + wrappedBox.getLabel());
    print("Wrapped box content: " + wrappedBox.getItem().getContent());

    // Triple nesting
    Box<Int> level1 = new Box<Int>(new Int(10));
    Box<Box<Int>> level2 = new Box<Box<Int>>(level1);
    Box<Box<Box<Int>>> level3 = new Box<Box<Box<Int>>>(level2);

    Box<Box<Int>> extracted2 = level3.getContent();
    Box<Int> extracted1 = extracted2.getContent();
    Int extracted0 = extracted1.getContent();
    print("Triple nested value: " + extracted0);

    // Nested with Wrapper
    Wrapper<Int> numWrapper = new Wrapper<Int>(new Int(99), "Number");
    Box<Wrapper<Int>> boxedWrapper = new Box<Wrapper<Int>>(numWrapper);
    Wrapper<Int> unwrapped = boxedWrapper.getContent();
    print("Boxed wrapper label: " + unwrapped.getLabel());
    print("Boxed wrapper value: " + unwrapped.getItem());

    print("Nested generic instantiation test completed");
}

main();
