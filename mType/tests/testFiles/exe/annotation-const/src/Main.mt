// MYT-376 round-trip: a const-folded annotation argument (60 * 1000) must
// survive full compilation to a standalone .exe and still be readable through
// reflection at runtime. Uses the reflection natives directly so the project
// stays self-contained (no external lib import in the .mtproj source tree).

annotation Timeout {
    int ms;
}

@Timeout(ms = 60 * 1000)
class Service { }

@EntryPoint
class App {
    public static function main(string[] args): void {
        int classHandle = __reflect_forName("Service");
        int annHandle = __reflect_getClassAnnotation(classHandle, "Timeout");
        int ms = __reflect_getAnnotationInt(annHandle, "ms");
        print("ms=" + ms);
        print("Annotation const fold round-trip passed");
    }
}
