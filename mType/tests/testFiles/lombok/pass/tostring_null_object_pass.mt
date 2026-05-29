// @ToString must render a null object field as the literal "null" rather than
// dereferencing it — `null.toString()` would crash at runtime. The no-arg
// constructor leaves the nullable `inner` field unset (null).
@NoArgsConstructor
class Inner {
}

@NoArgsConstructor
@ToString
class Holder {
    private Inner? inner;
}

Holder h = new Holder();
print(h.toString());
