// Module: Data pipeline interfaces

interface AsyncSource<T> {
    async fetch() : Promise<T[]>;
}

interface AsyncProcessor<T, R> {
    async process(data: T) : Promise<R>;
}

interface AsyncSink<T> {
    async write(data: T[]) : Promise<Int>;
}
