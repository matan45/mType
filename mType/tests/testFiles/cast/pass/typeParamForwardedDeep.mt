// MYT-228: 3-level forwarding chain. The valueKind=1 forwarding must
// cascade: outer<T> → middle<U> → inner<V>, all sharing the same concrete
// type. Each BIND_TYPE_ARGS at a level resolves the symbolic name against
// its caller's frame, so the innermost frame ends up with a concrete
// type even though the caller named a still-symbolic value at compile
// time.

class Dog {
    public constructor() {}
}

class Cat {
    public constructor() {}
}

class Chain {
    public static function <V> inner(Object x): bool {
        return x isClassOf V;
    }

    public static function <U> middle(Object x): bool {
        return Chain::inner<U>(x);
    }

    public static function <T> outer(Object x): bool {
        return Chain::middle<T>(x);
    }
}

Dog d = new Dog();
Cat c = new Cat();

print(Chain::outer<Dog>(d));  // true
print(Chain::outer<Dog>(c));  // false
print(Chain::outer<Cat>(d));  // false
print(Chain::outer<Cat>(c));  // true
