// MathLib: Interfaces for cross-library interface testing

interface Measurable {
    function measure(): float;
}

interface Printable {
    function display(): string;
}

interface Transformer<T> {
    function transform(T input): T;
}
