// Module: Async generic interface for comprehensive testing

interface AsyncMapper<T, R> {
    async map(items: T[], transformer: (T) -> R) : Promise<R[]>;
    async mapAsync(items: T[], transformer: (T) -> Promise<R>) : Promise<R[]>;
}

interface AsyncFilter<T> {
    async filter(items: T[], predicate: (T) -> Bool) : Promise<T[]>;
    async filterAsync(items: T[], predicate: (T) -> Promise<Bool>) : Promise<T[]>;
}
