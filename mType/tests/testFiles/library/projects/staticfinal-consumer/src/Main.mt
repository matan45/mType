// MYT-325 regression consumer:
// - Declares StaticFinalLib as a <Dependency> in the .mtproj (causes the
//   build to copy StaticFinalLib.mtcLib into build/libs/ as a sidecar).
// - Carries its own copy of ShaderType.mt in src/ (so the class is also
//   compiled into the consumer's embedded bytecode).
//
// Pre-fix: both BytecodeProgram instances carry ShaderType + its
//   "<static_init>$static" function. At launch the sidecar's initializer
//   runs first, writes the final fields, then the embedded program's
//   initializer runs and throws "Cannot assign to final static field".
// Post-fix: per-name dedup in VirtualMachine::runStaticInitializers skips
//   the second invocation.

@EntryPoint
class App {
    public static function main(string[] args): void {
        print("VERTEX=" + ShaderType::VERTEX);
        print("FRAGMENT=" + ShaderType::FRAGMENT);
        print("GEOMETRY=" + ShaderType::GEOMETRY);
        print("MYT-325 regression test passed");
    }
}
