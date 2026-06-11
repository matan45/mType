// CANARY (MYT-385): literals just above INT32_MAX wrap to negative because
// ExpressionParser.cpp:863 truncates Token::intValue through static_cast<int>.
// This test pins the CORRECT output and stays failing until MYT-385 lands.
function main(): void {
    print(2147483648);
    print(4294967296);
}
main();

// Expected output:
// 2147483648
// 4294967296
