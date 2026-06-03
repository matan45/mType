// MYT-376: per-element folding inside a string[] literal, mixing a concat
// expression with a `Class::FIELD` string constant.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Names {
    string[] tags;
}

class Cfg {
    public static final string X = "ex";
}

@Names(tags = ["a" + "b", Cfg::X])
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Names");
if (a != null) {
    string[] s = a.getStringArray("tags");
    for (int i = 0; i < s.length; i = i + 1) {
        print(s[i]);
    }
}
