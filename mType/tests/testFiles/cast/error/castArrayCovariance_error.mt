// Error: Array covariance - Object[] to String[] is not type safe
// In mType, array types are not covariant to prevent runtime type errors

class Object {}
class String {}


function testArrayCovariance(): void {
    Object[] objects = new Object[3];
    objects[0] = new Object();
    objects[1] = new Object();
    objects[2] = new Object();

    // Error: Cannot cast Object[] to String[]
    // This would allow putting non-String objects into a String array
    String[] strings = (String[])objects;
}
testArrayCovariance();
