// MYT-220: (T?)null cast syntax rejected by parser.
//
// EXPECTED:
//   The cast (Holder?)null is parsed and yields a typed null value.
//   Useful for disambiguating overloads that take different nullable types.
//
// ACTUAL (broken):
//   Parse error: Unexpected token in primary expression
//   (The `?` after the type name in cast position is not accepted, even
//   though `Holder? h = ...;` is a valid declaration form.)

class Holder {}

Holder? h = (Holder?)null;
if (h == null) {
    print("null cast ok");
}
