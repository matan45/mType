// MYT-375: positional shorthand (@Pair(5)) is only valid on annotations with
// exactly one declared parameter.
annotation Pair {
    int a;
    int b;
}

@Pair(5)
class Bad { }
