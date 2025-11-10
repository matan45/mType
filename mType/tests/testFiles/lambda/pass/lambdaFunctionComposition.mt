import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Function composition test
interface Function<T, R> {
    function apply(T input) : R;
}

class Composer {
    public function <A, B, C> compose(Function<B, C> f, Function<A, B> g) : Function<A, C> {
        Function<A, C> composed = x -> {
            B intermediate = g.apply(x);
            return f.apply(intermediate);
        };
        return composed;
    }
}

print("=== Function Composition Test ===");

Composer comp = new Composer();

// f(x) = x * 2
Function<Int, Int> double = x -> x * 2;

// g(x) = x + 10
Function<Int, Int> add10 = x -> x + 10;

// h(x) = f(g(x)) = (x + 10) * 2
Function<Int, Int> composed1 = comp.compose<Int, Int, Int>(double, add10);
print("composed1(5): " + composed1.apply(5));  // (5+10)*2 = 30
print("composed1(10): " + composed1.apply(10));  // (10+10)*2 = 40

// h(x) = g(f(x)) = (x * 2) + 10
Function<Int, Int> composed2 = comp.compose<Int, Int, Int>(add10, double);
print("composed2(5): " + composed2.apply(5).toString());  // (5*2)+10 = 20
print("composed2(10): " + composed2.apply(10).toString());  // (10*2)+10 = 30

// Chain three functions
Function<Int, Int> square = x -> x * x;
Function<Int, Int> temp = comp.compose<Int, Int, Int>(square, add10);  // (x+10)^2
Function<Int, Int> composed3 = comp.compose<Int, Int, Int>(double, temp);  // 2*(x+10)^2

print("composed3(3): " + composed3.apply(3).toString());  // 2*(3+10)^2 = 2*169 = 338

// Compose with type transformation
Function<Int, String> intToString = x -> new String("Value:" + x.toString());
Function<String, Int> stringLength = s -> new Int(s.length());
Function<Int, Int> lengthAfterConvert = comp.compose<Int, String, Int>(stringLength, intToString);

print("Length of 'Value:42': " + lengthAfterConvert.apply(42).toString());
print("Length of 'Value:100': " + lengthAfterConvert.apply(100).toString());

print("Function composition complete");
