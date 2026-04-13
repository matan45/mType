class Result {
    public int value;
    public constructor(int v) {
        this.value = v;
    }
}

function async computeValue(): Promise<Result> {
    Result r = new Result(42);
    return r;
}

function async main(): Promise<Result> {
    Result result = await computeValue();
    print(result.value);
    return result;
}

main();
