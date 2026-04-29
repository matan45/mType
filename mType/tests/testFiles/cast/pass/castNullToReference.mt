// Test: a nullable reference variable can be assigned null and compared.
class Holder {}

Holder? h = null;
if (h == null) {
    print("null cast ok");
}
