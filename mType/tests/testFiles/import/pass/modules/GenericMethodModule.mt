class Processor {
    fun process<T>(item: T): T {
        return item;
    }

    fun convert<T, U>(from: T, converter: (T) -> U): U {
        return converter(from);
    }

    fun swap<T, U>(first: T, second: U): Array<Object> {
        var result = Array<Object>(2);
        result[0] = second;
        result[1] = first;
        return result;
    }
}
