// Test: casting null to a nullable reference type yields null
class Holder {}

Holder? h = (Holder?)null;
if (h == null) {
    print("null cast ok");
}
