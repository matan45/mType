---
title: Reflection
sidebar_position: 8
---

# Reflection

`lib/reflect/` provides first-class reflection over classes, methods, fields, constructors, and annotations.

## Types

| Type | File | Role |
|---|---|---|
| `Class` | `lib/reflect/Class.mt` | A reflected class declaration. |
| `Method` | `lib/reflect/Method.mt` | A reflected method on a class. |
| `Field` | `lib/reflect/Field.mt` | A reflected field. |
| `Constructor` | `lib/reflect/Constructor.mt` | A reflected constructor. |
| `Annotation` | `lib/reflect/Annotation.mt` | A reflected annotation instance. |
| `Library` | `lib/reflect/Library.mt` | Loaded `.mtcLib` introspection. |
| `Modifier` | `lib/reflect/Modifier.mt` | Access modifier flags. |

## Looking up a Class

```mtype
import * from "lib/reflect/Class.mt";

Class c = Class::forName("Service");
print(c.getName());
```

## Listing Methods

```mtype
import * from "lib/reflect/Class.mt";
import * from "lib/reflect/Method.mt";

Class c = Class::forName("Service");
ArrayList<Method> methods = c.getDeclaredMethods();
for (Method m : methods) {
    print(m.getName() + " : " + m.getReturnType().getName());
}
```

## Reading Annotations

Only `@Retention(RUNTIME)` annotations survive into runtime metadata.

```mtype
import * from "lib/reflect/Class.mt";
import * from "lib/reflect/Method.mt";
import * from "lib/reflect/Annotation.mt";

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping", 0);
Annotation? t = m.getAnnotation("Timeout");
if (t != null) {
    print(t.getInt("ms"));
}
```

`Annotation` exposes typed getters for scalar and array parameters:

| Method | Returns |
|---|---|
| `getInt(key)` | `int` |
| `getFloat(key)` | `float` |
| `getBool(key)` | `bool` |
| `getString(key)` | `string` |
| `getClass(key)` | reflected `Class` handle |
| `getClassArray(key)` | reflected `Class[]` handles |
| `getClassNames(key)` | `string[]` of class names |
| `getIntArray(key)` | `int[]` |
| `getFloatArray(key)` | `float[]` |
| `getBoolArray(key)` | `bool[]` |
| `getStringArray(key)` | `string[]` |
| `isNull(key)` | `true` when the annotation value is `null` |

```mtype
@Retention(RUNTIME)
annotation Uses {
    Class[] types;
    string[] names;
}

class A { }
class B { }

@Uses(types = [A, B], names = ["left", "right"])
class Target { }

Annotation? uses = Class::forName("Target").getAnnotation("Uses");
if (uses != null) {
    string[] typeNames = uses.getClassNames("types");
    string[] names = uses.getStringArray("names");
    print(typeNames[0] + ":" + names[0]);   // A:left
}
```

## Invocation

```mtype
Constructor ctor = c.getConstructor(0);
Object[] args = new Object[0];
Object instance = ctor.newInstance(args);

Method m = c.getDeclaredMethod("greet", 0);
m.invoke(instance, args);
```

`newInstance` and `invoke` are **instance** methods — call them with `.` on a `Constructor` or `Method` value returned from reflection. Both take a `Object[]` of arguments.

## See Also

- [Annotations](../language/annotations.md)
- [Architecture / VM](../architecture/vm.md) — class metadata in bytecode.
