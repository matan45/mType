// @Getter targets CLASS/FIELD only — applying it to a top-level function is a
// @Target violation.
@Getter
public function doStuff(): void {
    print("hi");
}

doStuff();
