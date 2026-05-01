// MYT-247: ++ on a `final` local must be a compile error.
function main(): void {
    final int n = 0;
    n++;
    print(n);
}
main();
