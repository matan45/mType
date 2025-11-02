// Module: Polymorphic async interfaces with generics

interface AsyncHandler<T> {
    async handle(input: T) : Promise<T>;
}

interface AsyncValidator<T> {
    async validate(input: T) : Promise<Bool>;
}

interface AsyncComposite<T> : AsyncHandler<T>, AsyncValidator<T> {
    async process(input: T, onValid: (T) -> T) : Promise<T>;
}
