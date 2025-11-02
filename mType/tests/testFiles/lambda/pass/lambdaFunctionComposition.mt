// Function composition test
interface Function<T, R> {
    function apply(T input) : R;
}

class Composer {
    function compose<A, B, C>(Function<B, C> f, Function<A, B> g) : Function<A, C> {
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
Function<int, int> double = x -> x * 2;

// g(x) = x + 10
Function<int, int> add10 = x -> x + 10;

// h(x) = f(g(x)) = (x + 10) * 2
Function<int, int> composed1 = comp.compose(double, add10);
print("composed1(5): " + composed1.apply(5));  // (5+10)*2 = 30
print("composed1(10): " + composed1.apply(10));  // (10+10)*2 = 40

// h(x) = g(f(x)) = (x * 2) + 10
Function<int, int> composed2 = comp.compose(add10, double);
print("composed2(5): " + composed2.apply(5));  // (5*2)+10 = 20
print("composed2(10): " + composed2.apply(10));  // (10*2)+10 = 30

// Chain three functions
Function<int, int> square = x -> x * x;
Function<int, int> temp = comp.compose(square, add10);  // (x+10)^2
Function<int, int> composed3 = comp.compose(double, temp);  // 2*(x+10)^2

print("composed3(3): " + composed3.apply(3));  // 2*(3+10)^2 = 2*169 = 338

// Compose with type transformation
Function<int, String> intToString = x -> "Value:" + x;
Function<String, int> stringLength = s -> s.length();
Function<int, int> lengthAfterConvert = comp.compose(stringLength, intToString);

print("Length of 'Value:42': " + lengthAfterConvert.apply(42));
print("Length of 'Value:100': " + lengthAfterConvert.apply(100));

print("Function composition complete");
