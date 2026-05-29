// @Builder targets CLASS only — applying it to a top-level function is a
// @Target violation.
@Builder
public function makeThing(): void {
    print("hi");
}

makeThing();
