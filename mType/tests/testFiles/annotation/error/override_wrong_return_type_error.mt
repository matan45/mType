// Test: @Override annotation with wrong return type
// Expected: Compile Error - return type doesn't match

class Parent {
    Int getValue() {
        return 42;
    }
}

class Child extends Parent {
    @Override
    String getValue() {
        return "This should fail - wrong return type";
    }
}

// This should not compile
Child child = new Child();
String value = child.getValue();
