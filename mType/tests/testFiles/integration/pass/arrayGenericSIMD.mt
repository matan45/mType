// Arrays + Generics Test 5: SIMD operations on generic arrays
@Script

class VectorOps<T> {
    public function addScalar(T[] arr, Int size, T scalar): void {
        Int i = 0;
        while (i < size) {
            arr[i] = arr[i] + scalar;
            i = i + 1;
        }
    }

    public function multiplyScalar(T[] arr, Int size, T scalar): void {
        Int i = 0;
        while (i < size) {
            arr[i] = arr[i] * scalar;
            i = i + 1;
        }
    }

    public function dotProduct(T[] arr1, T[] arr2, Int size): T {
        T result = arr1[0] * arr2[0];
        result = result - result; // Zero out
        Int i = 0;
        while (i < size) {
            result = result + (arr1[i] * arr2[i]);
            i = i + 1;
        }
        return result;
    }

    public function vectorAdd(T[] result, T[] arr1, T[] arr2, Int size): void {
        Int i = 0;
        while (i < size) {
            result[i] = arr1[i] + arr2[i];
            i = i + 1;
        }
    }

    public function sum(T[] arr, Int size): T {
        T result = arr[0];
        result = result - result; // Zero out
        Int i = 0;
        while (i < size) {
            result = result + arr[i];
            i = i + 1;
        }
        return result;
    }
}

VectorOps<Int> intOps = VectorOps<Int>();
Int[] vec1 = Int[5];
vec1[0] = 1;
vec1[1] = 2;
vec1[2] = 3;
vec1[3] = 4;
vec1[4] = 5;

print("Original vector:");
Int i = 0;
while (i < 5) {
    print(vec1[i]);
    i = i + 1;
}

intOps.addScalar(vec1, 5, 10);
print("After adding 10:");
i = 0;
while (i < 5) {
    print(vec1[i]);
    i = i + 1;
}

Int[] vec2 = Int[5];
vec2[0] = 2;
vec2[1] = 3;
vec2[2] = 4;
vec2[3] = 5;
vec2[4] = 6;

Int dot = intOps.dotProduct(vec1, vec2, 5);
print("Dot product:");
print(dot);

Int[] vec3 = Int[5];
intOps.vectorAdd(vec3, vec1, vec2, 5);
print("Vector addition:");
i = 0;
while (i < 5) {
    print(vec3[i]);
    i = i + 1;
}

Int sum = intOps.sum(vec3, 5);
print("Sum of result:");
print(sum);

VectorOps<Float> floatOps = VectorOps<Float>();
Float[] fvec = Float[3];
fvec[0] = 1.5;
fvec[1] = 2.5;
fvec[2] = 3.5;

floatOps.multiplyScalar(fvec, 3, 2.0);
print("Float vector * 2.0:");
i = 0;
while (i < 3) {
    print(fvec[i]);
    i = i + 1;
}
