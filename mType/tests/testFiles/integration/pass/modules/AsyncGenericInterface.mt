// Module: Async generic interface for comprehensive testing

interface AsyncMapper<T, R> {
    function async map(items: T[], transformer: (T) -> R) : Promise<R[]>;
    function async mapAsync(items: T[], transformer: (T) -> Promise<R>) : Promise<R[]>;
}

interface AsyncFilter<T> {
    function async filter(items: T[], predicate: (T) -> Bool) : Promise<T[]>;
    function async filterAsync(items: T[], predicate: (T) -> Promise<Bool>) : Promise<T[]>;
}
