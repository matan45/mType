// MYT-325 regression fixture: class with public static final fields.
// The class is compiled into StaticFinalLib.mtcLib AND, when imported by
// the consumer via a source-path import, also into the consumer's embedded
// bytecode. The launcher loading both must not double-execute the static
// initializer (which would trip the final-field write check).
class ShaderType {
    public static final int VERTEX = 0;
    public static final int FRAGMENT = 1;
    public static final int GEOMETRY = 2;
}
