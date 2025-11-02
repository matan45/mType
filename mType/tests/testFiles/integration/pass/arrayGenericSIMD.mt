// Arrays + Generics Test 5: SIMD operations on generic arrays
@Script

class VectorOps<T> {
    fun addScalar(arr: T[], size: Int, scalar: T): Void {
        let i: Int = 0;
        while (i < size) {
            arr[i] = arr[i] + scalar;
            i = i + 1;
        }
    }

    fun multiplyScalar(arr: T[], size: Int, scalar: T): Void {
        let i: Int = 0;
        while (i < size) {
            arr[i] = arr[i] * scalar;
            i = i + 1;
        }
    }

    fun dotProduct(arr1: T[], arr2: T[], size: Int): T {
        let result: T = arr1[0] * arr2[0];
        result = result - result; // Zero out
        let i: Int = 0;
        while (i < size) {
            result = result + (arr1[i] * arr2[i]);
            i = i + 1;
        }
        return result;
    }

    fun vectorAdd(result: T[], arr1: T[], arr2: T[], size: Int): Void {
        let i: Int = 0;
        while (i < size) {
            result[i] = arr1[i] + arr2[i];
            i = i + 1;
        }
    }

    fun sum(arr: T[], size: Int): T {
        let result: T = arr[0];
        result = result - result; // Zero out
        let i: Int = 0;
        while (i < size) {
            result = result + arr[i];
            i = i + 1;
        }
        return result;
    }
}

let intOps: VectorOps<Int> = VectorOps<Int>();
let vec1: Int[] = Int[5];
vec1[0] = 1;
vec1[1] = 2;
vec1[2] = 3;
vec1[3] = 4;
vec1[4] = 5;

print("Original vector:");
let i: Int = 0;
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

let vec2: Int[] = Int[5];
vec2[0] = 2;
vec2[1] = 3;
vec2[2] = 4;
vec2[3] = 5;
vec2[4] = 6;

let dot: Int = intOps.dotProduct(vec1, vec2, 5);
print("Dot product:");
print(dot);

let vec3: Int[] = Int[5];
intOps.vectorAdd(vec3, vec1, vec2, 5);
print("Vector addition:");
i = 0;
while (i < 5) {
    print(vec3[i]);
    i = i + 1;
}

let sum: Int = intOps.sum(vec3, 5);
print("Sum of result:");
print(sum);

let floatOps: VectorOps<Float> = VectorOps<Float>();
let fvec: Float[] = Float[3];
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
