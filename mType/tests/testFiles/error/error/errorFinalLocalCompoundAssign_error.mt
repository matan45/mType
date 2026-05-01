// MYT-247: compound assignment to a `final` local must be a compile error.
function main(): void {
    final int n = 0;
    n += 5;
    print(n);
}
main();
