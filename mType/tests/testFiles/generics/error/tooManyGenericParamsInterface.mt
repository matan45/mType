// Test: Too many generic parameters on interface (> 20)
// Should fail at parse time with clear error message

interface TooManyParams<A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U> {
    function test(): void;
}
