// MYT-374: obj?.field = value writes when the receiver is non-null and is a
// no-op (yielding null) when it is null — no NullPointerException.

class Box {
    public string label;
    constructor(string l) { this.label = l; }
}

function main(): void {
    Box? present = new Box("old");
    Box? absent = null;

    present?.label = "updated";   // receiver non-null -> writes
    absent?.label = "ignored";    // receiver null -> skipped, no crash

    if (present != null) {
        print("present: " + present.label);
    }
    print("done");
}

main();

// Expected output:
// present: updated
// done
