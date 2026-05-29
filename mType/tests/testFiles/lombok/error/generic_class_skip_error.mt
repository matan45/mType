// Generic classes are skipped by Lombok synthesis (consistent with MYT-274),
// so no getValue() is generated for Box<T> and the call fails to resolve.
@Getter
class Box<T> {
    private T value;
}

Box<int> b = new Box<int>();
print(b.getValue());
