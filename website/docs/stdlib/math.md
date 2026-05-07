---
title: Math
sidebar_position: 6
---

# Math

`lib/math/` provides linear algebra primitives and a random number generator.

| Type | File |
|---|---|
| `Vec2f`, `Vec3f`, `Vec4f` | `lib/math/Vec{2,3,4}f.mt` |
| `Matrix3f`, `Matrix4f` | `lib/math/Matrix{3,4}f.mt` |
| `Quaternion` | `lib/math/Quaternion.mt` |
| `Random` | `lib/math/Random.mt` |

## Vectors

```mtype
import * from "lib/math/Vec3f.mt";

Vec3f a = new Vec3f(1.0, 2.0, 3.0);
Vec3f b = new Vec3f(4.0, 5.0, 6.0);
Vec3f sum = a.add(b);
float dot = a.dot(b);
print(sum.toString());
print(dot);
```

Common methods: `add`, `subtract`, `scale`, `dot`, `cross` (3D), `length`, `normalize`, `toString`.

## Matrices

Static factories use `::`; instance methods use `.`.

```mtype
import * from "lib/math/Matrix4f.mt";

Matrix4f identity    = Matrix4f::identity();              // static factory
Matrix4f translation = Matrix4f::translate(1.0, 2.0, 3.0); // static factory
Matrix4f combined    = identity.multiply(translation);    // instance method
```

## Quaternion

```mtype
import * from "lib/math/Quaternion.mt";

Quaternion q = Quaternion::fromAxisAngle(new Vec3f(0.0, 1.0, 0.0), 1.5708);
Vec3f rotated = q.rotate(new Vec3f(1.0, 0.0, 0.0));
```

## Random

```mtype
import * from "lib/math/Random.mt";

Random r = new Random(12345);  // seed
print(r.nextInt(100));         // 0..99
print(r.nextFloat());          // 0.0..1.0
print(r.nextBool());
```

## See Also

- [Standard Library / Primitives](primitives.md)
