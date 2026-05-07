// MYT-285: Interface declarations inside function bodies must be rejected
// at parse time. This test should fail to parse with a message naming the
// "function bodies are not allowed" rule.

function main(): void {
    interface Formatter {
        function format(string value): string;
    }
}

main();
