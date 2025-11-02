// Module: Error handling interfaces

interface AsyncFallible<T, E> {
    async execute() : Promise<T>;
    async onError(error: E) : Promise<T>;
}

interface AsyncRetryable {
    async retry(maxAttempts: Int) : Promise<Bool>;
}
