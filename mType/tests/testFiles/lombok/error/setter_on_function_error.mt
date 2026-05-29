// @Setter targets CLASS/FIELD only — applying it to a top-level function is a
// @Target violation.
@Setter
public function doIt(): void {
    print("hi");
}

doIt();
