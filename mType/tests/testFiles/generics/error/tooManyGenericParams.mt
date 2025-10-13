// Test: Too many generic parameters (> 20)
// Should fail at parse time with clear error message

class TooManyParams<A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U> {
    A data;
}
