public interface Converter<T, U> {
    function apply(T input): U;
}

public class Processor {
    public function <T> process(T item): T {
        return item;
    }

    public function <T, U> convert(T input, Converter<T, U> converter): U {
        return converter.apply(input);
    }
}
