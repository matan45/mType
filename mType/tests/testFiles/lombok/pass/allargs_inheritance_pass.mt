// @AllArgsConstructor walks the parent chain: Dog's constructor takes the
// inherited `name` plus its own `breed`, forwarding `name` to super(...).
// @ToString folds inherited + own fields.
@AllArgsConstructor
class Animal {
    protected string name;
}

@ToString
@AllArgsConstructor
class Dog extends Animal {
    private string breed;
}

Dog d = new Dog("Rex", "Lab");
print(d.toString());
