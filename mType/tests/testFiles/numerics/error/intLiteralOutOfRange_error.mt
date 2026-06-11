// Error: an integer literal that exceeds INT64 range is a lex/parse error
// ("out of range"), not a silent wrap.
function main(): void {
    print(99999999999999999999);
}
main();
