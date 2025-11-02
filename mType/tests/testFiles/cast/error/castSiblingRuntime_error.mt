@Script
@Throw
// Test sibling class cast at runtime (error)
class Parent {
    fn getName(): String {
        return "Parent";
    }
}

class ChildA : Parent {
    fn getName(): String {
        return "ChildA";
    }
}

class ChildB : Parent {
    fn getName(): String {
        return "ChildB";
    }
}

fn main() {
    let childA: ChildA = new ChildA();
    let parent: Parent = childA as Parent;  // Upcast OK

    // This should fail at runtime - trying to cast to sibling class
    let childB: ChildB = parent as ChildB;  // Runtime error
    print(childB.getName());
}
