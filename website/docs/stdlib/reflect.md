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

## Invocation

```mtype
Method m = c.getDeclaredMethod("greet", 0);
Object instance = c.newInstance();
m.invoke(instance, []);
```

`Constructor.newInstance(args)` and `Method.invoke(target, args)` accept arrays of arguments.

## See Also

- [Annotations](../language/annotations.md)
- [Architecture / VM](../architecture/vm.md) — class metadata in bytecode.
