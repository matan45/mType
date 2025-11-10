import * from "../../lib/primitives/Int.mt";

// Ambiguous type inference
class Wrapper<T> {
    T data;

    public function setData(T d): void {
        data = d;
    }
}

function <T> createWrapper(): Wrapper<T> {
    return new Wrapper<T>();
}

function main(): void {
    // Error: Cannot infer T - no context available
    Wrapper wrapper = createWrapper();
    wrapper.setData(new Int(42));
}

main();
