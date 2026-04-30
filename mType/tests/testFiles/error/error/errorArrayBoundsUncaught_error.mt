// Array index out of bounds without a surrounding try/catch must
// surface as an uncaught runtime exception with "out of bounds" in
// the message.
function main(): void {
    int[] a = new int[3];
    a[0] = 1; a[1] = 2; a[2] = 3;
    int x = a[5];
    print(x);
}
main();
