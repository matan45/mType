// Test: a marker annotation (zero-parameter, RUNTIME retention, CLASS target)
// applied to a class is discoverable via reflection.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
@Target([CLASS])
annotation Marker { }

@Marker
class Tagged { }

class Untagged { }

Class taggedClass = Class::forName("Tagged");
Class untaggedClass = Class::forName("Untagged");

print("tagged hasAnnotation: " + taggedClass.hasAnnotation("Marker"));
print("untagged hasAnnotation: " + untaggedClass.hasAnnotation("Marker"));
print("tagged annotation count: " + taggedClass.getAnnotations().length);
print("untagged annotation count: " + untaggedClass.getAnnotations().length);

print("Test passed");
