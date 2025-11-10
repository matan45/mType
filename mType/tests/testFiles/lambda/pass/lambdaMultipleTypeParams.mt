// BiFunction with multiple type parameters
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface BiFunction<T, U, R> {
    function apply(T first, U second) : R;
}

print("=== Multiple Type Parameters Test ===");

// BiFunction<Int, Int, Int>
BiFunction<Int, Int, Int> adder = (a, b) -> new Int(a.getValue() + b.getValue());
Int five = new Int(5);
Int three = new Int(3);
print("Add 5 + 3 = " + adder.apply(five, three).getValue());

// BiFunction<String, Int, String>
BiFunction<String, Int, String> repeater = (s, count) -> {
    String result = new String("");
    for (int i = 0; i < count.getValue(); i = i + 1) {
        result = new String(result.getValue() + s.getValue());
    }
    return result;
};
String hi = new String("Hi");
Int three2 = new Int(3);
print("Repeat 'Hi' 3 times: " + repeater.apply(hi, three2).getValue());

// BiFunction<Int, String, String>
BiFunction<Int, String, String> formatter = (num, suffix) -> {
    return new String("Number " + num.getValue() + " " + suffix.getValue());
};
Int fortytwo = new Int(42);
String answer = new String("is the answer");
print(formatter.apply(fortytwo, answer).getValue());

// BiFunction with more complex logic
BiFunction<Int, Int, String> comparator = (a, b) -> {
    if (a.getValue() > b.getValue()) {
        return new String("first is greater");
    } else if (a.getValue() < b.getValue()) {
        return new String("second is greater");
    } else {
        return new String("equal");
    }
};
Int ten = new Int(10);
Int five2 = new Int(5);
Int three3 = new Int(3);
Int eight = new Int(8);
Int seven = new Int(7);
Int seven2 = new Int(7);
print("Compare 10, 5: " + comparator.apply(ten, five2).getValue());
print("Compare 3, 8: " + comparator.apply(three3, eight).getValue());
print("Compare 7, 7: " + comparator.apply(seven, seven2).getValue());

print("Multiple type params complete");
