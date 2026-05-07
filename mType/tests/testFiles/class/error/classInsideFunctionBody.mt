// MYT-285: Class declarations inside function bodies must be rejected
// at parse time. This test should fail to parse with a message naming the
// "function bodies are not allowed" rule.

function main(): void {
    class Local {
        public int x;
    }
}

main();
