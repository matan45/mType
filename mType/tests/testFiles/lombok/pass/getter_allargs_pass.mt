// @Getter generates getX() accessors; @AllArgsConstructor supplies a
// constructor over every instance field. Reads back via the synthesized getters.
@Getter
@AllArgsConstructor
class Person {
    private string name;
    private int age;
}

Person p = new Person("Bob", 30);
print(p.getName());
print(p.getAge());
