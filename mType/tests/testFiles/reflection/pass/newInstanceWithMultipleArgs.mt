// Constructor.newInstance(Object[]) wrapper path with 3+ args of mixed types.
// reflectionInvocation.mt only covers a 1-arg int constructor via the native
// directly. This drives the wrapper path through Object[], exercising the
// public reflection API surface.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Constructor.mt";

class Tag {
    public string text;
    public constructor(string s) {
        this.text = s;
    }
}

class Triple {
    public Tag first;
    public Tag second;
    public Tag third;

    public constructor(Tag a, Tag b, Tag c) {
        this.first = a;
        this.second = b;
        this.third = c;
    }

    public function describe(): string {
        return this.first.text + "/" + this.second.text + "/" + this.third.text;
    }
}

Class tripleClass = Class::forName("Triple");
Constructor ctor = tripleClass.getDeclaredConstructor(3);
print("ctor param count=" + ctor.getParameterCount());

Object[] args = new Object[3];
args[0] = new Tag("alpha");
args[1] = new Tag("beta");
args[2] = new Tag("gamma");

Object created = ctor.newInstance(args);
print("created non-null=" + (created != null));

// Read fields back through reflection (avoids needing a downcast).
import * from "../../lib/reflect/Field.mt";
Field firstField = tripleClass.getDeclaredField("first");
Object firstTag = firstField.get(created);
print("first non-null=" + (firstTag != null));

Class tagClass = Class::forName("Tag");
Field tagText = tagClass.getDeclaredField("text");
print("first.text=" + tagText.get(firstField.get(created)));
print("second.text=" + tagText.get(tripleClass.getDeclaredField("second").get(created)));
print("third.text=" + tagText.get(tripleClass.getDeclaredField("third").get(created)));

print("Test passed");
