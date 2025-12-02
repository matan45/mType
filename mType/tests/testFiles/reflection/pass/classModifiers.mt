// Test class modifier flags

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Modifier.mt";

abstract class AbstractClass {
    public int x;
}

final class FinalClass {
    public int y;
}

class NormalClass {
    public int z;
}

// Test abstract class modifiers
Class abstractClass = Class::forName("AbstractClass");
int abstractMods = abstractClass.getModifiers();
print("AbstractClass modifiers: " + abstractMods);
print("AbstractClass is abstract: " + Modifier::isAbstract(abstractMods));
print("AbstractClass is final: " + Modifier::isFinal(abstractMods));

// Test final class modifiers
Class finalClass = Class::forName("FinalClass");
int finalMods = finalClass.getModifiers();
print("FinalClass modifiers: " + finalMods);
print("FinalClass is abstract: " + Modifier::isAbstract(finalMods));
print("FinalClass is final: " + Modifier::isFinal(finalMods));

// Test normal class modifiers
Class normalClass = Class::forName("NormalClass");
int normalMods = normalClass.getModifiers();
print("NormalClass modifiers: " + normalMods);
print("NormalClass is abstract: " + Modifier::isAbstract(normalMods));
print("NormalClass is final: " + Modifier::isFinal(normalMods));

print("Test passed");
