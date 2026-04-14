// MYT-108: unknown annotation on a top-level function must error.
// Confirms the typed usage validator fires for non-class functions too.

@NotDeclared
function doWork(): void {
    print("never");
}

doWork();
