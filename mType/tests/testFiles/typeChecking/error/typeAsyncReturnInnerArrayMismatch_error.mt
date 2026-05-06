// Tier A: async function declared Promise<int[]> returns an int (after
// async-wrap) that should be wrapped to Promise<int>, not int[]. The
// Promise-extraction logic at FunctionCompiler.cpp:329-350 may not catch
// inner-type mismatches.
// Targets: FunctionCompiler.cpp:329-350.

function async getCounts(): Promise<int[]> {
    // Returns a single int, not an int[]. Async wrap would produce
    // Promise<int>, not Promise<int[]>.
    return 42;
}

function async main(): Promise<int> {
    int[] arr = await getCounts();
    return arr[0];
}

main();
