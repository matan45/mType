// Generic classes are skipped by Lombok synthesis (consistent with MYT-274),
// so no getValue() is generated for Box<T> and the call fails to resolve.
// (Uses a user-class type argument — a primitive like Box<int> would be
// rejected at the generic instantiation before reaching the getValue() call.)
@Getter
class Box<T> {
    private T value;
}

class Item {
    public int id;
}

Box<Item> b = new Box<Item>();
print(b.getValue());
