// @Data targets CLASS only — applying it to a top-level function is a @Target
// violation.
@Data
public function compute(): int {
    return 1;
}

print(compute());
