// CANARY (MYT-385): the INT64_MAX literal currently prints -1 because
// ExpressionParser.cpp:863 truncates Token::intValue through static_cast<int>.
// This test pins the CORRECT output and stays failing until MYT-385 lands.
function main(): void {
    print(9223372036854775807);
}
main();

// Expected output:
// 9223372036854775807
