// Error: float division by zero raises "Division by zero" (mType does not
// produce IEEE infinity from division; only overflow saturates to inf).
function divide(float a, float b): float {
    return a / b;
}
function main(): void {
    print(divide(10.0, 0.0));
}
main();
