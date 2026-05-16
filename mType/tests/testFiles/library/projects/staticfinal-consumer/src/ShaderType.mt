// MYT-325 regression: deliberately duplicates the class defined in
// staticfinal-lib so that, when this consumer is built as --exe with
// StaticFinalLib declared as a <Dependency>, ShaderType ends up in BOTH
// the consumer's embedded bytecode AND the sidecar StaticFinalLib.mtcLib.
// The launcher runs the static initializer from each program; the per-name
// dedup in VirtualMachine::runStaticInitializers prevents the second run
// from tripping the final-field write check.
class ShaderType {
    public static final int VERTEX = 0;
    public static final int FRAGMENT = 1;
    public static final int GEOMETRY = 2;
}
