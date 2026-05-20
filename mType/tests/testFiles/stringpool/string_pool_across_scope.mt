// Test: Strings declared in different scopes compare equal
function buildInFunction(): string {
    string local = "shared_token";
    return local;
}

class Holder {
    public string value;
    public constructor() {
        this.value = "shared_token";
    }
}

function main(): void {
    string atTop = "shared_token";
    string fromFn = buildInFunction();
    print("top eq fn: " + (atTop == fromFn));

    Holder h = new Holder();
    print("top eq field: " + (atTop == h.value));
    print("fn eq field: " + (fromFn == h.value));
}
main();

// Expected output:
// top eq fn: true
// top eq field: true
// fn eq field: true
