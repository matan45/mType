#include "LibraryTestSuite.hpp"
#include <cstddef>
#include <cstdint>
#include "../../services/ScriptInterpreter.hpp"
#include "../../services/ScriptAPI.hpp"
#include "../../value/ValueShim.hpp"
#include "../../types/TypeSignature.hpp"
#include "../../project/mtclib/MtcLibFormat.hpp"
#include "../../project/mtclib/MtcLibSerializer.hpp"
#include "../../project/mtclib/MtcLibBuilder.hpp"
#include "../../project/mtclib/MtcPathResolver.hpp"
#include "../../project/mtclib/DependencyResolver.hpp"
#include "../../project/mtclib/LibraryLinker.hpp"
#include "../../project/mtclib/LibrarySymbolProvider.hpp"
#include "../../project/mtclib/TransitiveDependencyLoader.hpp"
#include "../../project/mtclib/LibraryNatives.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../project/ProjectConfig.hpp"
#include "../../project/ProjectBuilder.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../environment/EnvironmentBuilder.hpp"
#include "../../environment/NativeContext.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../../version/Version.hpp"
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>
#include <initializer_list>

namespace tests::testSuite
{
    using namespace testFramework;
    using namespace services;

    namespace
    {
        void require(bool cond, const std::string& msg)
        {
            if (!cond) throw std::runtime_error(msg);
        }

        void requireEq(const std::string& expected, const std::string& actual, const std::string& context)
        {
            if (expected != actual)
                throw std::runtime_error(context + ": expected '" + expected + "', got '" + actual + "'");
        }

        // Invoke a NativeFunction (NativeDelegate) the way the VM does — with a
        // NativeContext + span. Replaces the legacy initializer_list call form
        // that worked when NativeFunction was std::function<Value(const vector<Value>&)>.
        value::Value invokeNative(
            const environment::registry::NativeFunction& fn,
            std::shared_ptr<environment::Environment> env,
            std::shared_ptr<vm::runtime::VirtualMachine> vm,
            std::initializer_list<value::Value> args)
        {
            environment::NativeContext ctx{ std::move(env), std::move(vm) };
            std::vector<value::Value> argsVec(args);
            return fn(ctx, std::span<const value::Value>(argsVec.data(), argsVec.size()));
        }
    }

    void LibraryTestSuite::setupTests()
    {
        setupSerializationTests();
        setupTypeSignatureTests();
        setupDependencyResolverTests();
        setupMtcLibBuilderTests();
        setupBuildTests();
        setupLinkingTests();
        setupRuntimeLoadingTests();
        setupTransitiveDependencyTests();
        setupNativeLoadLibraryTests();
        setupUnloadLibraryTests();
        setupClassTests();
        setupInterfaceTests();
        setupGenericsTests();
        setupArrayTests();
        setupLambdaTests();
        setupAsyncTests();
        setupCastTests();
        setupImportTests();
        setupCollectionTests();
        setupReflectionTests();
        setupValueClassTests();
        setupForEachTests();
        setupTryCatchTests();
        setupStaticTests();
        setupInheritanceTests();
        setupModifierTests();
        setupStreamTests();
        setupPatternMatchingTests();
    }

    // =========================================================================
    // MYT-67/68: Serialization unit tests
    // =========================================================================

    void LibraryTestSuite::setupSerializationTests()
    {
        using namespace vm::bytecode;

        addCallbackTest("BytecodeProgram isAbstract round-trips through serialization",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;

                // Add a class with an abstract method
                BytecodeProgram::ClassMetadata classMeta;
                classMeta.name = "TestClass";
                classMeta.isAbstract = true;

                BytecodeProgram::MethodMetadata method;
                method.name = "abstractMethod";
                method.returnType = "void";
                method.isStatic = false;
                method.isFinal = false;
                method.isPrivate = false;
                method.isProtected = false;
                method.isAbstract = true;
                method.startOffset = 0;
                classMeta.instanceMethods.push_back(method);

                prog.registerClass(classMeta);
                prog.setEntryPoint(0);

                // Emit a single HALT-like instruction so serialize doesn't fail
                prog.emit(OpCode::RETURN);

                // Round-trip
                std::stringstream ss;
                prog.serialize(ss);
                auto deserialized = BytecodeProgram::deserialize(ss);

                auto& classes = deserialized.getClasses();
                require(classes.size() == 1, "Expected 1 class");
                require(classes[0].isAbstract, "Class should be abstract");
                require(classes[0].instanceMethods.size() == 1, "Expected 1 method");
                require(classes[0].instanceMethods[0].isAbstract, "Method should be abstract");
            });

        addCallbackTest("InterfaceMetadata round-trips through serialization",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;

                BytecodeProgram::InterfaceMetadata ifaceMeta;
                ifaceMeta.name = "Comparable";
                ifaceMeta.genericParameters = {"T"};
                ifaceMeta.extendsInterfaces = {};
                ifaceMeta.isFinal = false;

                BytecodeProgram::InterfaceMethodSignature sig;
                sig.name = "compareTo";
                sig.returnType = "int";
                sig.parameterTypes = {"Object"};
                sig.parameterNames = {"other"};
                sig.genericTypeParameters = {};
                ifaceMeta.methods.push_back(sig);

                prog.addInterface(ifaceMeta);
                prog.setEntryPoint(0);
                prog.emit(OpCode::RETURN);

                // Round-trip
                std::stringstream ss;
                prog.serialize(ss);
                auto deserialized = BytecodeProgram::deserialize(ss);

                auto& interfaces = deserialized.getInterfaces();
                require(interfaces.size() == 1, "Expected 1 interface");
                requireEq("Comparable", interfaces[0].name, "Interface name");
                require(interfaces[0].genericParameters.size() == 1, "Expected 1 generic param");
                requireEq("T", interfaces[0].genericParameters[0], "Generic param name");
                require(interfaces[0].methods.size() == 1, "Expected 1 method");
                requireEq("compareTo", interfaces[0].methods[0].name, "Method name");
                requireEq("int", interfaces[0].methods[0].returnType, "Return type");
            });

        // MYT-286: peephole shrinking instructions inside a function body
        // leaves that function's recorded instructionCount stale (too large).
        // Pre-fix, deserialize at load rejected start+count > instructions.size()
        // and the user's RTSCameraController.mtcLib failed to load. Fix: clamp
        // instructionCount at serialize time so the on-disk range always fits.
        addCallbackTest("Stale FunctionMetadata.instructionCount round-trips (MYT-286)",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;

                // Layout: JUMP at 0, then a 5-instruction "function body" at [1..6).
                prog.emit(OpCode::JUMP, 6);
                prog.emit(OpCode::NOP);   // 1
                prog.emit(OpCode::NOP);   // 2  <- will be removed by replaceInstructions
                prog.emit(OpCode::NOP);   // 3  <- will be removed by replaceInstructions
                prog.emit(OpCode::NOP);   // 4
                prog.emit(OpCode::HALT);  // 5

                BytecodeProgram::FunctionMetadata meta;
                meta.name = "Inner::method";
                meta.mangledName = meta.name;
                meta.startOffset = 1;
                meta.instructionCount = 5;
                meta.localCount = 0;
                meta.parameterCount = 1;
                meta.parameterNames = {"this"};
                meta.returnType = "void";
                meta.isStatic = false;
                meta.isNative = false;
                prog.registerFunction(meta.name, meta);
                prog.setEntryPoint(0);

                // Simulate peephole shrinking the body by 1 (replace 2 NOPs with
                // 1 NOP) without updating instructionCount — this is exactly the
                // state the RTSCameraController constructor was in on disk.
                std::vector<BytecodeProgram::Instruction> replacement;
                BytecodeProgram::Instruction r;
                r.opcode = OpCode::NOP;
                replacement.push_back(r);
                prog.replaceInstructions(2, 2, replacement);
                prog.updateFunctionOffsets(2 + 2, -1);  // current behavior: only shifts later startOffsets

                // In-memory: instructionCount is still 5 (stale). The body
                // [1, 6) overflows the now-5-instruction array by exactly 1.
                const auto* f = prog.getFunction("Inner::method");
                require(f != nullptr, "Function should be registered");
                require(f->instructionCount == 5, "In-memory count is intentionally stale");
                require(prog.getInstructionCount() == 5, "Array should have shrunk by 1");

                // Round-trip — the writer must clamp the count so deserialize
                // doesn't reject the body as out-of-range. Pre-fix this throws.
                std::stringstream ss;
                prog.serialize(ss);
                auto deserialized = BytecodeProgram::deserialize(ss);

                const auto* g = deserialized.getFunction("Inner::method");
                require(g != nullptr, "Function should round-trip");
                require(g->startOffset + g->instructionCount
                            <= deserialized.getInstructionCount(),
                    "Deserialized body must fit within the instruction array");
            });

        // MYT-286: end-to-end repro. Mirrors C:\matan\RTSDemo\scripts\game\
        // RTSCameraController.mt — @Script class with many inline-initialized
        // fields and an empty user-defined constructor, run through the real
        // compile pipeline (with release-mode peephole) and then deserialized
        // from disk. Pre-fix this throws "out-of-range body" at deserialize
        // because the constructor's recorded instructionCount goes stale after
        // peephole shrinks the body.
        addCallbackTest("@Script empty-ctor class round-trips through .mtc on disk (MYT-286)",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                std::string sourcePath =
                    "mType/tests/testFiles/library/pass/rtsCameraStyleEmptyCtor.mt";
                require(fs::exists(sourcePath),
                    "fixture not found at: " + sourcePath);

                fs::path tempDir = fs::temp_directory_path() / "myt286";
                fs::create_directories(tempDir);
                std::string outPath = (tempDir / "rtsCameraStyleEmptyCtor.mtc").string();
                if (fs::exists(outPath)) fs::remove(outPath);

                // RAII cleanup so the temp dir is removed even if a require()
                // below throws. Captures by value to avoid dangling refs.
                struct ScopedRemove {
                    fs::path dir;
                    ~ScopedRemove() { std::error_code ec; fs::remove_all(dir, ec); }
                } cleanup{tempDir};

                // Compile through the production pipeline (lexer/parser/compiler/
                // optimizer/peephole) and write the .mtc.
                {
                    ScriptInterpreter interpreter;
                    interpreter.compileToFile(sourcePath, outPath);
                }
                require(fs::exists(outPath), ".mtc not produced at: " + outPath);

                // Deserialize. Pre-fix this throws because the empty
                // constructor's instructionCount overflowed instructions.size().
                std::ifstream in(outPath, std::ios::binary);
                require(in.good(), "cannot open compiled .mtc");
                auto reloaded = BytecodeProgram::deserialize(in);
                in.close();

                // Sanity-check the constructor metadata is consistent with the
                // instruction array we just loaded.
                bool foundCtor = false;
                for (const auto& [name, meta] : reloaded.getFunctions()) {
                    if (name.find("RTSCameraStyleScript::<init>") == 0) {
                        foundCtor = true;
                        require(meta.startOffset + meta.instructionCount
                                    <= reloaded.getInstructionCount(),
                            "ctor body must fit in instruction array: " + name);
                    }
                }
                require(foundCtor, "expected to find RTSCameraStyleScript::<init>");
            });

        // MYT-286 edge: function whose body shrinks ALL the way down to zero
        // (peephole removes everything between the entry and the trailing
        // RETURN). The clamp must produce a non-negative count.
        addCallbackTest("Peephole shrink-to-empty body still serializes (MYT-286 edge)",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;
                prog.emit(OpCode::JUMP, 4);  // 0  skip-jump
                prog.emit(OpCode::NOP);       // 1  body inst 1
                prog.emit(OpCode::NOP);       // 2  body inst 2
                prog.emit(OpCode::NOP);       // 3  body inst 3
                prog.emit(OpCode::HALT);      // 4

                BytecodeProgram::FunctionMetadata meta;
                meta.name = "Empty::method";
                meta.mangledName = meta.name;
                meta.startOffset = 1;
                meta.instructionCount = 3;
                meta.parameterCount = 1;
                meta.parameterNames = {"this"};
                meta.returnType = "void";
                prog.registerFunction(meta.name, meta);
                prog.setEntryPoint(0);

                // Wipe the entire body — replace 3 instructions with none.
                prog.replaceInstructions(1, 3, {});
                prog.updateFunctionOffsets(1 + 3, -3);
                require(prog.getInstructionCount() == 2,
                    "Array should be JUMP + HALT only");

                std::stringstream ss;
                prog.serialize(ss);
                auto reloaded = BytecodeProgram::deserialize(ss);
                const auto* f = reloaded.getFunction("Empty::method");
                require(f != nullptr, "Function survives empty-body shrink");
                require(f->startOffset + f->instructionCount
                            <= reloaded.getInstructionCount(),
                    "Empty body fits inside array (count clamped to 0 or less)");
            });

        // MYT-286 edge: function whose body sits at the very end of the
        // instructions array — there's no padding past startOffset+count for a
        // stale count to "spill into," so the clamp is the only thing keeping
        // serialize honest. (Real-world: RTSCameraController's <init> was the
        // last function before HALT; this is the same shape.)
        addCallbackTest("Peephole shrink at end-of-program clamps correctly (MYT-286 edge)",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;
                prog.emit(OpCode::JUMP, 5);   // 0
                prog.emit(OpCode::NOP);        // 1  body
                prog.emit(OpCode::NOP);        // 2  body  <- removed
                prog.emit(OpCode::NOP);        // 3  body  <- removed
                prog.emit(OpCode::NOP);        // 4  body
                prog.emit(OpCode::HALT);       // 5

                BytecodeProgram::FunctionMetadata meta;
                meta.name = "Tail::method";
                meta.mangledName = meta.name;
                meta.startOffset = 1;
                meta.instructionCount = 4;
                meta.parameterCount = 1;
                meta.parameterNames = {"this"};
                meta.returnType = "void";
                prog.registerFunction(meta.name, meta);
                prog.setEntryPoint(0);

                std::vector<BytecodeProgram::Instruction> one(1);
                one[0].opcode = OpCode::NOP;
                prog.replaceInstructions(2, 2, one);
                prog.updateFunctionOffsets(2 + 2, -1);

                // In-memory the count is still 4; on-disk must be clamped.
                std::stringstream ss;
                prog.serialize(ss);
                auto reloaded = BytecodeProgram::deserialize(ss);
                const auto* f = reloaded.getFunction("Tail::method");
                require(f != nullptr, "function round-trips");
                require(f->startOffset == 1, "startOffset preserved");
                require(f->startOffset + f->instructionCount
                            <= reloaded.getInstructionCount(),
                    "clamped count fits in deserialized array");
            });

        // MYT-286 edge: native functions have instructionCount = 0 and
        // isNative = true. The clamp's `!isNative` guard must NOT touch them —
        // their startOffset is a placeholder and any clamp arithmetic would be
        // meaningless. (No bug today; this test pins the guard so a future
        // refactor doesn't accidentally drop the isNative check.)
        addCallbackTest("Native function metadata is not clamped on serialize",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;
                prog.emit(OpCode::HALT);

                BytecodeProgram::FunctionMetadata native;
                native.name = "ext::nativeFn";
                native.mangledName = native.name;
                native.startOffset = 999999;   // bogus, won't fit in any array
                native.instructionCount = 0;
                native.parameterCount = 0;
                native.returnType = "void";
                native.isNative = true;
                prog.registerFunction(native.name, native);
                prog.setEntryPoint(0);

                std::stringstream ss;
                prog.serialize(ss);
                auto reloaded = BytecodeProgram::deserialize(ss);
                const auto* f = reloaded.getFunction("ext::nativeFn");
                require(f != nullptr, "native function round-trips");
                require(f->isNative, "isNative preserved");
                require(f->startOffset == 999999,
                    "native startOffset preserved verbatim (no clamp)");
                require(f->instructionCount == 0,
                    "native instructionCount preserved");
            });

        // MYT-286 edge: peephole INSERTS instructions (delta > 0). The clamp
        // only applies when start+count overflows; growth must pass through
        // untouched. Without my fix this case wasn't broken either, but it
        // pins the boundary behavior so a tighter clamp later doesn't
        // accidentally shrink a perfectly valid body.
        addCallbackTest("Peephole insertion preserves on-disk instructionCount",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;
                prog.emit(OpCode::JUMP, 4);   // 0
                prog.emit(OpCode::NOP);        // 1
                prog.emit(OpCode::NOP);        // 2  <- replace with 2 instrs
                prog.emit(OpCode::NOP);        // 3
                prog.emit(OpCode::HALT);       // 4

                BytecodeProgram::FunctionMetadata meta;
                meta.name = "Grow::method";
                meta.mangledName = meta.name;
                meta.startOffset = 1;
                meta.instructionCount = 3;
                meta.parameterCount = 1;
                meta.parameterNames = {"this"};
                meta.returnType = "void";
                prog.registerFunction(meta.name, meta);
                prog.setEntryPoint(0);

                std::vector<BytecodeProgram::Instruction> two(2);
                two[0].opcode = OpCode::NOP;
                two[1].opcode = OpCode::NOP;
                prog.replaceInstructions(2, 1, two);
                prog.updateFunctionOffsets(2 + 1, +1);

                require(prog.getInstructionCount() == 6, "array grew by 1");

                std::stringstream ss;
                prog.serialize(ss);
                auto reloaded = BytecodeProgram::deserialize(ss);
                const auto* f = reloaded.getFunction("Grow::method");
                require(f != nullptr, "function round-trips after growth");
                // In-memory count is still 3 (peephole's updateFunctionOffsets
                // does not grow either), and start+count = 4 <= size 6, so the
                // clamp is a no-op and the original 3 is what we should see.
                require(f->instructionCount == 3,
                    "growth path leaves count untouched (no clamp triggered)");
            });

        // MYT-286 adversarial: two functions, peephole shrinks INSIDE the
        // second one. The first must round-trip with its original count
        // unchanged, the second's stale count must be clamped on disk.
        addCallbackTest("Two functions, mod inside second — first preserved",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;
                prog.emit(OpCode::JUMP, 4);   // 0   skip fn1
                prog.emit(OpCode::NOP);        // 1   fn1 body
                prog.emit(OpCode::NOP);        // 2   fn1 body
                prog.emit(OpCode::NOP);        // 3   fn1 body
                prog.emit(OpCode::JUMP, 8);   // 4   skip fn2
                prog.emit(OpCode::NOP);        // 5   fn2 body
                prog.emit(OpCode::NOP);        // 6   fn2 body  <- shrunk
                prog.emit(OpCode::NOP);        // 7   fn2 body  <- shrunk
                prog.emit(OpCode::HALT);       // 8

                BytecodeProgram::FunctionMetadata fn1;
                fn1.name = "Two::first";
                fn1.mangledName = fn1.name;
                fn1.startOffset = 1;
                fn1.instructionCount = 3;
                fn1.parameterCount = 1;
                fn1.parameterNames = {"this"};
                fn1.returnType = "void";
                prog.registerFunction(fn1.name, fn1);

                BytecodeProgram::FunctionMetadata fn2;
                fn2.name = "Two::second";
                fn2.mangledName = fn2.name;
                fn2.startOffset = 5;
                fn2.instructionCount = 3;
                fn2.parameterCount = 1;
                fn2.parameterNames = {"this"};
                fn2.returnType = "void";
                prog.registerFunction(fn2.name, fn2);
                prog.setEntryPoint(0);

                std::vector<BytecodeProgram::Instruction> one(1);
                one[0].opcode = OpCode::NOP;
                prog.replaceInstructions(6, 2, one);  // shrink inside fn2
                prog.updateFunctionOffsets(6 + 2, -1);

                std::stringstream ss;
                prog.serialize(ss);
                auto reloaded = BytecodeProgram::deserialize(ss);

                const auto* f1 = reloaded.getFunction("Two::first");
                const auto* f2 = reloaded.getFunction("Two::second");
                require(f1 != nullptr && f2 != nullptr, "both functions present");
                require(f1->startOffset == 1 && f1->instructionCount == 3,
                    "fn1 untouched by mod inside fn2");
                require(f2->startOffset + f2->instructionCount
                            <= reloaded.getInstructionCount(),
                    "fn2 body fits in array on reload");
            });

        // MYT-286 adversarial: adversarially-constructed input with a function
        // whose startOffset is past instructions.size(). The deserializer
        // already rejects this (independent check at readFunctions); the test
        // pins the behavior so the clamp doesn't mask a genuine corruption.
        addCallbackTest("Deserializer still rejects out-of-range startOffset",
            "",
            [](ScriptAPI&) {
                BytecodeProgram prog;
                prog.emit(OpCode::HALT);  // single instruction

                BytecodeProgram::FunctionMetadata meta;
                meta.name = "Bogus::method";
                meta.mangledName = meta.name;
                meta.startOffset = 9999;   // way past array end
                meta.instructionCount = 0;
                meta.parameterCount = 1;
                meta.parameterNames = {"this"};
                meta.returnType = "void";
                meta.isNative = false;     // critical: not native, so validated
                prog.registerFunction(meta.name, meta);
                prog.setEntryPoint(0);

                std::stringstream ss;
                prog.serialize(ss);
                bool threw = false;
                try {
                    auto reloaded = BytecodeProgram::deserialize(ss);
                } catch (const std::runtime_error&) {
                    threw = true;
                }
                require(threw,
                    "deserializer must reject a non-native function whose "
                    "startOffset is past the instruction array end");
            });
    }

    // =========================================================================
    // MYT-69: TypeSignature unit tests
    // =========================================================================

    void LibraryTestSuite::setupTypeSignatureTests()
    {
        using namespace vm::bytecode;

        addCallbackTest("TypeSignature encodes class with generics",
            "",
            [](ScriptAPI&) {
                BytecodeProgram::ClassMetadata meta;
                meta.name = "HashMap";
                meta.genericParameters = {"K", "V"};

                std::string sig = ::types::TypeSignature::encodeClassSignature(meta);
                require(sig.find("HashMap<K,V>") != std::string::npos,
                    "Expected 'HashMap<K,V>' in signature, got: " + sig);
            });

        addCallbackTest("TypeSignature encodes interface signature",
            "",
            [](ScriptAPI&) {
                BytecodeProgram::InterfaceMetadata meta;
                meta.name = "Iterable";
                meta.genericParameters = {"T"};
                meta.extendsInterfaces = {"Collection"};

                std::string sig = ::types::TypeSignature::encodeInterfaceSignature(meta);
                require(sig.find("Iterable<T>") != std::string::npos,
                    "Expected 'Iterable<T>' in signature, got: " + sig);
                require(sig.find("extends Collection") != std::string::npos,
                    "Expected 'extends Collection' in signature, got: " + sig);
            });

        addCallbackTest("TypeSignature encodes function signature",
            "",
            [](ScriptAPI&) {
                BytecodeProgram::FunctionMetadata meta;
                meta.name = "add";
                meta.returnType = "int";
                meta.parameterTypes = {"int", "int"};
                meta.parameterNames = {"a", "b"};

                std::string sig = ::types::TypeSignature::encodeFunctionSignature(meta);
                requireEq("add(int,int):int", sig, "Function signature");
            });

        addCallbackTest("TypeSignature exact match is compatible",
            "",
            [](ScriptAPI&) {
                require(::types::TypeSignature::signaturesCompatible("int", "int"),
                    "Same signature should be compatible");
                require(!::types::TypeSignature::signaturesCompatible("int", "float"),
                    "Different signatures should not be compatible");
            });
    }

    // =========================================================================
    // MYT-70: MtcLibSerializer round-trip tests
    // =========================================================================

    void LibraryTestSuite::setupMtcLibBuilderTests()
    {
        addCallbackTest("MtcLibSerializer round-trips MtcLibProgram",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;
                using namespace vm::bytecode;

                // Build a minimal MtcLibProgram
                MtcLibProgram lib;
                lib.header = MtcLibHeader{};
                lib.metadata.name = "testlib";
                lib.metadata.version = "1.0.0";
                lib.metadata.mtypeVersion = mType::version::getVersionString();
                lib.metadata.sourceHash = 42;

                MtcLibDependency dep;
                dep.name = "base";
                dep.versionConstraint = "^1.0.0";
                dep.contentHash = 99;
                lib.dependencies.push_back(dep);

                MtcLibExport exp;
                exp.kind = SymbolKind::FUNCTION;
                exp.name = "add";
                exp.signature = "add(int,int):int";
                exp.bytecodeOffset = 0;
                exp.visibility = SymbolVisibility::PUBLIC;
                lib.exports.push_back(exp);

                MtcLibImport imp;
                imp.kind = SymbolKind::CLASS;
                imp.name = "Base";
                imp.signature = "Base";
                imp.sourceLib = "base";
                lib.imports.push_back(imp);

                // Embed a minimal BytecodeProgram
                lib.bytecodeProgram.setEntryPoint(0);
                lib.bytecodeProgram.emit(OpCode::RETURN);

                // Round-trip through serialization
                std::stringstream ss;
                MtcLibSerializer::serialize(lib, ss);
                auto deserialized = MtcLibSerializer::deserialize(ss);

                // Verify header
                require(std::memcmp(deserialized.header.magic, "MTCLIB", 6) == 0,
                    "Magic should be MTCLIB");
                require(deserialized.header.version == 1, "Version should be 1");

                // Verify metadata
                requireEq("testlib", deserialized.metadata.name, "Library name");
                requireEq("1.0.0", deserialized.metadata.version, "Library version");
                require(deserialized.metadata.sourceHash == 42, "Source hash should be 42");

                // Verify dependencies
                require(deserialized.dependencies.size() == 1, "Expected 1 dependency");
                requireEq("base", deserialized.dependencies[0].name, "Dependency name");

                // Verify exports
                require(deserialized.exports.size() == 1, "Expected 1 export");
                requireEq("add", deserialized.exports[0].name, "Export name");

                // Verify imports
                require(deserialized.imports.size() == 1, "Expected 1 import");
                requireEq("Base", deserialized.imports[0].name, "Import name");
            });

        addCallbackTest("MtcLibSerializer rejects bad magic",
            "",
            [](ScriptAPI&) {
                std::stringstream ss;
                ss.write("BADMAG", 6);
                uint16_t version = 1;
                ss.write(reinterpret_cast<const char*>(&version), sizeof(version));
                uint32_t flags = 0;
                ss.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

                bool threw = false;
                try {
                    project::mtclib::MtcLibSerializer::deserialize(ss);
                } catch (const std::runtime_error& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("bad magic") != std::string::npos,
                        "Error should mention bad magic, got: " + msg);
                }
                require(threw, "Should throw on bad magic");
            });
    }

    // =========================================================================
    // MYT-72: DependencyResolver unit tests
    // =========================================================================

    void LibraryTestSuite::setupDependencyResolverTests()
    {
        addCallbackTest("DependencyResolver topological sort with linear chain",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                std::unordered_map<std::string, MtcLibProgram> libs;

                // A depends on B, B depends on C
                MtcLibProgram a;
                a.metadata.name = "A";
                a.dependencies.push_back({"B", "1.0", 0});
                libs["A"] = std::move(a);

                MtcLibProgram b;
                b.metadata.name = "B";
                b.dependencies.push_back({"C", "1.0", 0});
                libs["B"] = std::move(b);

                MtcLibProgram c;
                c.metadata.name = "C";
                libs["C"] = std::move(c);

                DependencyResolver resolver;
                auto result = resolver.resolve(libs);

                require(result.success, "Resolution should succeed");
                require(result.loadOrder.size() == 3, "Should have 3 libs in order");

                // C must come before B, B before A
                size_t posC = 0, posB = 0, posA = 0;
                for (size_t i = 0; i < result.loadOrder.size(); ++i) {
                    if (result.loadOrder[i] == "C") posC = i;
                    if (result.loadOrder[i] == "B") posB = i;
                    if (result.loadOrder[i] == "A") posA = i;
                }
                require(posC < posB, "C should load before B");
                require(posB < posA, "B should load before A");
            });

        addCallbackTest("DependencyResolver detects circular dependency",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                std::unordered_map<std::string, MtcLibProgram> libs;

                MtcLibProgram a;
                a.metadata.name = "A";
                a.dependencies.push_back({"B", "1.0", 0});
                libs["A"] = std::move(a);

                MtcLibProgram b;
                b.metadata.name = "B";
                b.dependencies.push_back({"A", "1.0", 0});
                libs["B"] = std::move(b);

                DependencyResolver resolver;
                auto result = resolver.resolve(libs);

                require(!result.success, "Should fail with circular dependency");
                require(result.errorMessage.find("Circular") != std::string::npos,
                    "Error should mention circular, got: " + result.errorMessage);
            });

        addCallbackTest("DependencyResolver detects version conflict",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                std::unordered_map<std::string, MtcLibProgram> libs;

                // A needs C v1.0, B needs C v2.0
                MtcLibProgram a;
                a.metadata.name = "A";
                a.dependencies.push_back({"C", "1.0", 0});
                libs["A"] = std::move(a);

                MtcLibProgram b;
                b.metadata.name = "B";
                b.dependencies.push_back({"C", "2.0", 0});
                libs["B"] = std::move(b);

                MtcLibProgram c;
                c.metadata.name = "C";
                libs["C"] = std::move(c);

                DependencyResolver resolver;
                auto result = resolver.resolve(libs);

                require(!result.success, "Should fail with version conflict");
                require(result.errorMessage.find("conflict") != std::string::npos ||
                        result.errorMessage.find("Version") != std::string::npos,
                    "Error should mention conflict, got: " + result.errorMessage);
            });

        addCallbackTest("DependencyResolver handles diamond deduplication",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                std::unordered_map<std::string, MtcLibProgram> libs;

                // A and B both depend on C v1.0 (same version = no conflict)
                MtcLibProgram a;
                a.metadata.name = "A";
                a.dependencies.push_back({"C", "1.0", 0});
                libs["A"] = std::move(a);

                MtcLibProgram b;
                b.metadata.name = "B";
                b.dependencies.push_back({"C", "1.0", 0});
                libs["B"] = std::move(b);

                MtcLibProgram c;
                c.metadata.name = "C";
                libs["C"] = std::move(c);

                DependencyResolver resolver;
                auto result = resolver.resolve(libs);

                require(result.success, "Should succeed with same-version diamond");
                // C should appear exactly once in load order
                int cCount = 0;
                for (const auto& name : result.loadOrder) {
                    if (name == "C") cCount++;
                }
                require(cCount == 1, "C should appear exactly once, got " + std::to_string(cCount));
            });
    }

    // =========================================================================
    // MYT-71: MtcLibBuilder tests
    // =========================================================================

    void LibraryTestSuite::setupBuildTests()
    {
        addCallbackTest("Build MathLib .mtcLib from .mtproj",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                std::string mtprojPath = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(mtprojPath), "MathLib.mtproj not found at: " + mtprojPath);

                // Parse project config
                project::ProjectConfigParser parser;
                auto config = parser.parse(mtprojPath);
                require(config != nullptr, "Failed to parse MathLib.mtproj");
                require(!config->resolvedSourceFiles.empty(), "No source files resolved");

                // Build library
                fs::path outputDir = fs::path(config->projectRoot) / config->output.directory;
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(*config, libPath);

                require(result.success, "Library build failed: " +
                    (result.errors.empty() ? "unknown error" : result.errors[0]));
                require(fs::exists(libPath), ".mtcLib file not created at: " + libPath);

                // Verify the file has correct magic bytes
                std::ifstream inFile(libPath, std::ios::binary);
                require(inFile.good(), "Cannot open .mtcLib file");
                char magic[6];
                inFile.read(magic, 6);
                require(std::memcmp(magic, "MTCLIB", 6) == 0,
                    "Bad magic in .mtcLib file");
                inFile.close();

                // Deserialize and verify contents
                std::ifstream verifyFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(verifyFile);
                verifyFile.close();

                requireEq("MathLib", lib.metadata.name, "Library name");
                require(lib.metadata.sourceHash != 0, "Source hash should be computed (non-zero)");
                require(!lib.exports.empty(), "Library should have exports");

                // Check that key symbols are exported
                bool foundMathUtils = false;
                bool foundVector2 = false;
                bool foundShape = false;
                for (const auto& exp : lib.exports) {
                    if (exp.name == "MathUtils") foundMathUtils = true;
                    if (exp.name == "Vector2") foundVector2 = true;
                    if (exp.name == "Shape") foundShape = true;
                }
                require(foundMathUtils, "MathUtils should be exported");
                require(foundVector2, "Vector2 should be exported");
                require(foundShape, "Shape should be exported");

                // Cleanup
                fs::remove(libPath);
                fs::remove_all(outputDir);
            });
    }

    // =========================================================================
    // MYT-72-74: Compile-time linking tests
    // =========================================================================

    void LibraryTestSuite::setupLinkingTests()
    {
        addCallbackTest("MtcPathResolver finds library in search path",
            "",
            [](ScriptAPI&) {
                project::mtclib::MtcPathResolver resolver(".");
                auto result = resolver.resolve("nonexistent-lib");
                require(!result.has_value(), "Should not find nonexistent library");
            });

        // MYT-323: MtcPathResolver must consume the mtpm registry layout
        // <root>/<name>/<version>/<name>.mtcLib so packages installed by
        // mtpm and compiled by PackageCompiler are linkable without a flat
        // libs/ folder.
        addCallbackTest("MtcPathResolver: registry layout resolves versioned lookup",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path tempRegistry = fs::temp_directory_path() / "_mtype_resolver_test_registry";
                if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
                fs::create_directories(tempRegistry / "myt323_demolib" / "1.2.3");

                fs::path artifact = tempRegistry / "myt323_demolib" / "1.2.3" / "myt323_demolib.mtcLib";
                { std::ofstream f(artifact, std::ios::binary); f << "fake-bytecode"; }

                project::mtclib::MtcPathResolver resolver(".");
                resolver.addRegistryRoot(tempRegistry.string());

                auto resolved = resolver.resolve("myt323_demolib", "1.2.3");
                auto cleanup = [&]() { fs::remove_all(tempRegistry); };

                if (!resolved.has_value()) {
                    cleanup();
                    throw std::runtime_error("Should resolve myt323_demolib@1.2.3 from registry");
                }
                if (fs::canonical(*resolved) != fs::canonical(artifact)) {
                    cleanup();
                    throw std::runtime_error("Expected " + artifact.string() + ", got " + *resolved);
                }
                cleanup();
            });

        addCallbackTest("MtcPathResolver: registry layout strips version range prefix",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path tempRegistry = fs::temp_directory_path() / "_mtype_resolver_test_range";
                if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
                fs::create_directories(tempRegistry / "myt323_demolib" / "1.2.3");

                fs::path artifact = tempRegistry / "myt323_demolib" / "1.2.3" / "myt323_demolib.mtcLib";
                { std::ofstream f(artifact, std::ios::binary); f << "fake"; }

                project::mtclib::MtcPathResolver resolver(".");
                resolver.addRegistryRoot(tempRegistry.string());

                // Real .mtproj files often say "^1.2.3" — without stripping the
                // caret, resolver would fail to find the exact-version dir.
                auto resolved = resolver.resolve("myt323_demolib", "^1.2.3");
                auto cleanup = [&]() { fs::remove_all(tempRegistry); };

                if (!resolved.has_value()) {
                    cleanup();
                    throw std::runtime_error("Should resolve through stripped range prefix");
                }
                cleanup();
            });

        addCallbackTest("MtcPathResolver: registry layout picks highest version when unversioned",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path tempRegistry = fs::temp_directory_path() / "_mtype_resolver_test_highest";
                if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);

                // Three versions on disk; resolve() with no constraint should pick 2.0.0
                for (const auto& v : {"1.0.0", "1.5.0", "2.0.0"}) {
                    fs::create_directories(tempRegistry / "myt323_demolib" / v);
                    fs::path artifact = tempRegistry / "myt323_demolib" / v / "myt323_demolib.mtcLib";
                    std::ofstream f(artifact, std::ios::binary); f << "fake";
                }

                project::mtclib::MtcPathResolver resolver(".");
                resolver.addRegistryRoot(tempRegistry.string());

                auto resolved = resolver.resolve("myt323_demolib");
                auto cleanup = [&]() { fs::remove_all(tempRegistry); };

                if (!resolved.has_value()) {
                    cleanup();
                    throw std::runtime_error("Should resolve unversioned myt323_demolib");
                }
                if (resolved->find("2.0.0") == std::string::npos) {
                    cleanup();
                    throw std::runtime_error("Expected highest version 2.0.0, got " + *resolved);
                }
                cleanup();
            });

        addCallbackTest("MtcPathResolver: flat libs/ dir still works (regression)",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path tempDir = fs::temp_directory_path() / "_mtype_resolver_test_flat";
                if (fs::exists(tempDir)) fs::remove_all(tempDir);
                fs::create_directories(tempDir);

                fs::path artifact = tempDir / "myt323_flatlib.mtcLib";
                { std::ofstream f(artifact, std::ios::binary); f << "fake"; }

                project::mtclib::MtcPathResolver resolver(".");
                resolver.addSearchPath(tempDir.string());

                auto resolved = resolver.resolve("myt323_flatlib");
                auto cleanup = [&]() { fs::remove_all(tempDir); };

                if (!resolved.has_value()) {
                    cleanup();
                    throw std::runtime_error("Flat layout regression: should resolve myt323_flatlib");
                }
                cleanup();
            });

        // MYT-323: LibraryLinker is now the .mtcLib code path only.
        // ProjectBuilder filters out deps that already have mt_modules source
        // before calling the linker — so reaching the throw means neither
        // source nor bytecode exists. The error advises both alternatives.
        addCallbackTest("LibraryLinker: error names library and suggests mtpm install + .mtcLib",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path tempProject = fs::temp_directory_path() / "_mtype_linker_test_missing";
                if (fs::exists(tempProject)) fs::remove_all(tempProject);
                fs::create_directories(tempProject);

                project::mtclib::LibraryLinker linker(tempProject.string());
                project::ProjectConfig config;
                config.projectRoot = tempProject.string();
                project::PackageDependency dep;
                dep.name = "myt323_newlib";
                dep.versionRange = "1.0.0";
                config.dependencies.packages.push_back(dep);

                std::string errMsg;
                try
                {
                    linker.linkDependencies(config);
                }
                catch (const std::exception& e)
                {
                    errMsg = e.what();
                }

                auto cleanup = [&]() { fs::remove_all(tempProject); };

                if (errMsg.find("myt323_newlib") == std::string::npos)
                {
                    cleanup();
                    throw std::runtime_error("Error must name the missing library, got: " + errMsg);
                }
                if (errMsg.find("mtpm install") == std::string::npos)
                {
                    cleanup();
                    throw std::runtime_error("Expected 'mtpm install' hint, got: " + errMsg);
                }
                if (errMsg.find(".mtcLib") == std::string::npos)
                {
                    cleanup();
                    throw std::runtime_error("Error should mention the .mtcLib alternative");
                }
                cleanup();
            });

        // MYT-323: ProjectBuilder must skip LibraryLinker for any <Package>
        // dep that has source available in mt_modules/. This is the path
        // that unblocks `mType.exe --build` after `mtpm install` — without
        // it, the linker errors out trying to find a .mtcLib that mtpm
        // doesn't produce.
        addCallbackTest("ProjectBuilder: skips LibraryLinker for deps with mt_modules source",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path proj = fs::temp_directory_path() / "_mtype_pb_test_mtmod_skip";
                if (fs::exists(proj)) fs::remove_all(proj);
                fs::create_directories(proj / "mt_modules" / "@myt323_srcdep");
                fs::create_directories(proj / "src");

                // Minimal source — empty is fine, ProjectBuilder just needs
                // SOMETHING to bundle. Parse errors are tolerated; we only
                // care that the library-not-found error isn't raised.
                { std::ofstream f(proj / "src" / "Main.mt"); f << ""; }

                // Mark the mt_modules entry as installed per
                // MtModulesManager::isInstalled — that requires mtpkg.json.
                {
                    std::ofstream f(proj / "mt_modules" / "@myt323_srcdep" / "mtpkg.json");
                    f << R"({"name":"myt323_srcdep","version":"1.0.0"})";
                }

                project::ProjectConfig config;
                config.name = "test_skip";
                config.projectRoot = fs::canonical(proj).string();
                config.resolvedSourceFiles.push_back(
                    fs::canonical(proj / "src" / "Main.mt").string());
                project::PackageDependency dep;
                dep.name = "myt323_srcdep";
                dep.versionRange = "1.0.0";
                config.dependencies.packages.push_back(dep);

                project::ProjectBuilder builder;
                std::string outputPath = (proj / "out.mtcLib").string();
                auto result = builder.buildLibrary(config, outputPath);

                auto cleanup = [&]() { fs::remove_all(proj); };

                // KEY assertion: LibraryLinker must NOT have fired for the
                // mt_modules-backed dep. If it had, the result would carry
                // "Library 'myt323_srcdep' not found".
                for (const auto& err : result.errors)
                {
                    if (err.find("Library 'myt323_srcdep' not found") != std::string::npos)
                    {
                        cleanup();
                        throw std::runtime_error(
                            "LibraryLinker fired for dep with mt_modules source: " + err);
                    }
                }
                cleanup();
            });

        addCallbackTest("ProjectBuilder: still calls LibraryLinker for deps without mt_modules",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                fs::path proj = fs::temp_directory_path() / "_mtype_pb_test_mtmod_miss";
                if (fs::exists(proj)) fs::remove_all(proj);
                fs::create_directories(proj / "src");
                // NO mt_modules → dep falls through to LibraryLinker.

                { std::ofstream f(proj / "src" / "Main.mt"); f << ""; }

                project::ProjectConfig config;
                config.name = "test_miss";
                config.projectRoot = fs::canonical(proj).string();
                config.resolvedSourceFiles.push_back(
                    fs::canonical(proj / "src" / "Main.mt").string());
                project::PackageDependency dep;
                dep.name = "myt323_missingdep";
                dep.versionRange = "1.0.0";
                config.dependencies.packages.push_back(dep);

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(config, (proj / "out.mtcLib").string());

                auto cleanup = [&]() { fs::remove_all(proj); };

                // Must surface the linker error so the user knows to install.
                bool foundLinkerError = false;
                for (const auto& err : result.errors)
                {
                    if (err.find("myt323_missingdep") != std::string::npos &&
                        err.find("not found") != std::string::npos)
                    {
                        foundLinkerError = true;
                        break;
                    }
                }
                if (!foundLinkerError)
                {
                    std::string allErrors;
                    for (const auto& e : result.errors) allErrors += e + "; ";
                    cleanup();
                    throw std::runtime_error(
                        "Expected library-not-found error for dep without mt_modules; got: " + allErrors);
                }
                cleanup();
            });

        addCallbackTest("LibraryLinker: setLockfileVersions overrides declared range",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                // Layout: tempRegistry/myt323_lockedlib/0.0.1/myt323_lockedlib.mtcLib with valid bytes
                // declared range = "^1.0.0", lockfile pin = "0.0.1"
                // Without the pin, resolution would target the range. With the
                // pin, it lands on 0.0.1 — failure mode at deserialize is OK; we
                // only need to assert which path was attempted.
                fs::path tempRegistry = fs::temp_directory_path() / "_mtype_linker_test_pin";
                if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
                fs::create_directories(tempRegistry / "myt323_lockedlib" / "0.0.1");

                // Empty (invalid) .mtcLib — deserialize will throw, but the
                // throw comes from MtcLibSerializer (after pathResolver
                // succeeded), so the error text differs from "not found".
                fs::path artifact = tempRegistry / "myt323_lockedlib" / "0.0.1" / "myt323_lockedlib.mtcLib";
                { std::ofstream f(artifact, std::ios::binary); }

                project::mtclib::LibraryLinker linker(".");
                linker.getPathResolver().addRegistryRoot(tempRegistry.string());

                std::unordered_map<std::string, std::string> pins;
                pins["myt323_lockedlib"] = "0.0.1";
                linker.setLockfileVersions(pins);

                project::ProjectConfig config;
                config.projectRoot = ".";
                project::PackageDependency dep;
                dep.name = "myt323_lockedlib";
                dep.versionRange = "^1.0.0";  // would resolve nowhere without pin
                config.dependencies.packages.push_back(dep);

                std::string errMsg;
                try
                {
                    linker.linkDependencies(config);
                }
                catch (const std::exception& e)
                {
                    errMsg = e.what();
                }

                auto cleanup = [&]() { fs::remove_all(tempRegistry); };

                // The pin should have routed resolution into the registry —
                // expect a deserialization-style error, NOT "Library not found".
                if (errMsg.find("not found") != std::string::npos)
                {
                    cleanup();
                    throw std::runtime_error(
                        "setLockfileVersions did not pin to 0.0.1; got: " + errMsg);
                }
                cleanup();
            });

        addCallbackTest("LibrarySymbolProvider registers class stubs from built library",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Build MathLib
                std::string mtprojPath = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(mtprojPath), "MathLib.mtproj not found");

                project::ProjectConfigParser parser;
                auto config = parser.parse(mtprojPath);

                fs::path outputDir = fs::path(config->projectRoot) / config->output.directory;
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(*config, libPath);
                require(result.success, "Build failed: " +
                    (result.errors.empty() ? "unknown" : result.errors[0]));

                // Load and register symbols
                std::ifstream inFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();

                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(lib, env);

                // Verify classes registered
                require(env->findClass("MathUtils") != nullptr, "MathUtils should be registered");
                require(env->findClass("Vector2") != nullptr, "Vector2 should be registered");
                require(env->findClass("Shape") != nullptr, "Shape should be registered");
                require(env->findClass("Circle") != nullptr, "Circle should be registered");
                require(env->findClass("Rectangle") != nullptr, "Rectangle should be registered");

                // Verify interfaces registered
                auto ifaceRegistry = env->getInterfaceRegistry();
                require(ifaceRegistry != nullptr, "Interface registry should exist");
                require(ifaceRegistry->hasInterface("Measurable"), "Measurable interface should be registered");
                require(ifaceRegistry->hasInterface("Printable"), "Printable interface should be registered");
                require(ifaceRegistry->hasInterface("Transformer"), "Transformer interface should be registered");

                // Cleanup
                fs::remove(libPath);
                fs::remove_all(outputDir);
            });

        addCallbackTest("Selective import registers only selected symbols",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Build MathLib
                std::string mtprojPath = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser parser;
                auto config = parser.parse(mtprojPath);

                fs::path outputDir = fs::path(config->projectRoot) / config->output.directory;
                fs::create_directories(outputDir);
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(*config, libPath);
                require(result.success, "Build failed");

                // Load library
                std::ifstream inFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                // Register with selective filter: only Vector2 and MathUtils
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();

                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(
                    lib, env, {"Vector2", "MathUtils"});

                // Vector2 and MathUtils should be registered
                require(env->findClass("Vector2") != nullptr, "Vector2 should be registered");
                require(env->findClass("MathUtils") != nullptr, "MathUtils should be registered");

                // Shape, Circle, Rectangle should NOT be registered
                require(env->findClass("Shape") == nullptr, "Shape should NOT be registered");
                require(env->findClass("Circle") == nullptr, "Circle should NOT be registered");
                require(env->findClass("Rectangle") == nullptr, "Rectangle should NOT be registered");

                // Cleanup
                fs::remove(libPath);
                fs::remove_all(outputDir);
            });

        addCallbackTest("Selective import detects name collision",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Build MathLib
                std::string mtprojPath = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser parser;
                auto config = parser.parse(mtprojPath);

                fs::path outputDir = fs::path(config->projectRoot) / config->output.directory;
                fs::create_directories(outputDir);
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(*config, libPath);
                require(result.success, "Build failed");

                std::ifstream inFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                // Register all symbols first
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(lib, env);

                // Now try selective import of same symbol — should detect conflict
                bool threw = false;
                try {
                    project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(
                        lib, env, {"Vector2"});
                } catch (const std::runtime_error& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("conflict") != std::string::npos,
                        "Error should mention conflict, got: " + msg);
                }
                require(threw, "Should throw on name collision with selective import");

                // Cleanup
                fs::remove(libPath);
                fs::remove_all(outputDir);
            });

        addCallbackTest("Import with alias registers under alias name",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Build MathLib
                std::string mtprojPath = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser parser;
                auto config = parser.parse(mtprojPath);

                fs::path outputDir = fs::path(config->projectRoot) / config->output.directory;
                fs::create_directories(outputDir);
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(*config, libPath);
                require(result.success, "Build failed");

                std::ifstream inFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                // Register with aliases: Vector2 as Vec2, MathUtils as Math
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();

                std::unordered_map<std::string, std::string> aliases = {
                    {"Vector2", "Vec2"},
                    {"MathUtils", "Math"}
                };

                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(
                    lib, env, {"Vector2", "MathUtils"}, aliases);

                // Should be registered under alias names
                require(env->findClass("Vec2") != nullptr, "Vec2 (alias for Vector2) should be registered");
                require(env->findClass("Math") != nullptr, "Math (alias for MathUtils) should be registered");

                // Original names should NOT be registered
                require(env->findClass("Vector2") == nullptr, "Vector2 should NOT be registered (aliased to Vec2)");
                require(env->findClass("MathUtils") == nullptr, "MathUtils should NOT be registered (aliased to Math)");

                // Cleanup
                fs::remove(libPath);
                fs::remove_all(outputDir);
            });

        addCallbackTest("Alias resolves name collision between two libraries",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Build MathLib twice (simulating two different libs with same class names)
                std::string mtprojPath = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser parser;
                auto config = parser.parse(mtprojPath);

                fs::path outputDir = fs::path(config->projectRoot) / config->output.directory;
                fs::create_directories(outputDir);
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();

                project::ProjectBuilder builder;
                auto result = builder.buildLibrary(*config, libPath);
                require(result.success, "Build failed");

                std::ifstream inFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();

                // First import: Vector2 as MathVector
                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(
                    lib, env, {"Vector2"}, {{"Vector2", "MathVector"}});

                // Second import of same lib: Vector2 as PhysicsVector (no collision!)
                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(
                    lib, env, {"Vector2"}, {{"Vector2", "PhysicsVector"}});

                // Both aliases should exist
                require(env->findClass("MathVector") != nullptr, "MathVector should be registered");
                require(env->findClass("PhysicsVector") != nullptr, "PhysicsVector should be registered");

                // Original name should NOT exist
                require(env->findClass("Vector2") == nullptr, "Vector2 original should NOT be registered");

                // Cleanup
                fs::remove(libPath);
                fs::remove_all(outputDir);
            });
    }

    // =========================================================================
    // MYT-75: Runtime loading tests
    // =========================================================================

    void LibraryTestSuite::setupRuntimeLoadingTests()
    {
        addCallbackTest("ScriptInterpreter loadCompiledBytecode accepts .mtcLib as main program",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);
                require(libConfig != nullptr, "Failed to parse MathLib.mtproj");

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));

                ScriptInterpreter interpreter;
                interpreter.loadCompiledBytecode(libPath);

                value::Value result = interpreter.callStaticMethod(
                    "MathUtils",
                    "max",
                    { value::Value(int64_t{2}), value::Value(int64_t{5}) });

                require(value::isInt(result), "MathUtils.max should return int");
                require(value::asInt(result) == 5,
                    "MathUtils.max should return 5, got " + std::to_string(value::asInt(result)));

                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("ScriptInterpreter parseAndRegisterClasses accepts .mtcLib as main program",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);
                require(libConfig != nullptr, "Failed to parse MathLib.mtproj");

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));

                ScriptInterpreter interpreter;
                interpreter.parseAndRegisterClasses(libPath);

                value::Value result = interpreter.callStaticMethod(
                    "MathUtils",
                    "min",
                    { value::Value(int64_t{2}), value::Value(int64_t{5}) });

                require(value::isInt(result), "MathUtils.min should return int");
                require(value::asInt(result) == 2,
                    "MathUtils.min should return 2, got " + std::to_string(value::asInt(result)));

                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("End-to-end: build MathLib, load into interpreter, run consumer code",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // === Step 1: Build MathLib to .mtcLib ===
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(libMtproj), "MathLib.mtproj not found");

                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);
                require(libConfig != nullptr, "Failed to parse MathLib.mtproj");

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));
                require(fs::exists(libPath), ".mtcLib not created at: " + libPath);

                // === Step 2: Load .mtcLib and register symbols into a fresh interpreter ===
                std::ifstream inFile(libPath, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                requireEq("MathLib", lib.metadata.name, "Library name mismatch");
                require(!lib.exports.empty(), "Library should have exports");
                require(!lib.bytecodeProgram.getClasses().empty(), "Library should have class metadata");
                require(!lib.bytecodeProgram.getInterfaces().empty(), "Library should have interface metadata");

                // === Step 3: Verify symbol registration ===
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                project::mtclib::LibrarySymbolProvider::registerLibrarySymbols(lib, env);

                // Verify classes
                require(env->findClass("MathUtils") != nullptr, "MathUtils class should be registered");
                require(env->findClass("Vector2") != nullptr, "Vector2 class should be registered");
                require(env->findClass("Shape") != nullptr, "Shape class should be registered");
                require(env->findClass("Circle") != nullptr, "Circle class should be registered");
                require(env->findClass("Rectangle") != nullptr, "Rectangle class should be registered");

                // Verify interfaces
                auto ifaceReg = env->getInterfaceRegistry();
                require(ifaceReg->hasInterface("Measurable"), "Measurable should be registered");
                require(ifaceReg->hasInterface("Printable"), "Printable should be registered");
                require(ifaceReg->hasInterface("Transformer"), "Transformer should be registered");

                // Verify Vector2 class has expected methods
                auto vector2 = env->findClass("Vector2");
                require(vector2 != nullptr, "Vector2 should exist");

                // Verify MathUtils has static methods
                auto mathUtils = env->findClass("MathUtils");
                require(mathUtils != nullptr, "MathUtils should exist");

                // === Cleanup ===
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("End-to-end: consumer .mtproj builds as library with MathLib dependency",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Step 1: Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));

                // Step 2: Copy to consumer/libs/
                std::string consumerLibsDir = "mType/tests/testFiles/library/projects/consumer/libs";
                fs::create_directories(consumerLibsDir);
                std::string destLib = consumerLibsDir + "/MathLib.mtcLib";
                fs::copy_file(libPath, destLib, fs::copy_options::overwrite_existing);

                // Step 3: Build consumer as library (tests compile-time linking via buildLibrary)
                // buildLibrary internally calls compileToProgram which resolves <Dependencies>
                std::string consumerMtproj = "mType/tests/testFiles/library/projects/consumer/Consumer.mtproj";
                auto consumerConfig = configParser.parse(consumerMtproj);
                require(consumerConfig != nullptr, "Failed to parse Consumer.mtproj");

                fs::path consumerOutputDir = fs::path(consumerConfig->projectRoot) / consumerConfig->output.directory;
                fs::create_directories(consumerOutputDir);
                std::string consumerLibPath = (consumerOutputDir / (consumerConfig->name + ".mtcLib")).string();

                try {
                    project::ProjectBuilder consumerBuilder;
                    auto consumerResult = consumerBuilder.buildLibrary(*consumerConfig, consumerLibPath);

                    require(consumerResult.success, "Consumer build failed: " +
                        (consumerResult.errors.empty() ? "unknown" : consumerResult.errors[0]));
                    require(fs::exists(consumerLibPath), "Consumer .mtcLib not created");
                } catch (const std::exception& e) {
                    require(false, "Consumer compilation failed: " + std::string(e.what()));
                }

                // Cleanup
                try {
                    fs::remove(destLib);
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                    fs::remove(consumerLibPath);
                    fs::remove_all(consumerOutputDir);
                } catch (...) {}
            });

        addCallbackTest("End-to-end exe: build MathLib, build Consumer exe, run and verify output",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // === Step 1: Build MathLib .mtcLib ===
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(libMtproj), "MathLib.mtproj not found");

                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));

                // === Step 2: Copy .mtcLib to consumer/libs/ ===
                std::string consumerLibsDir = "mType/tests/testFiles/library/projects/consumer/libs";
                fs::create_directories(consumerLibsDir);
                std::string destLib = consumerLibsDir + "/MathLib.mtcLib";
                fs::copy_file(libPath, destLib, fs::copy_options::overwrite_existing);

                // === Step 3: Find launcher binary ===
#ifdef _WIN32
                std::string launcherName = "mtype-launcher.exe";
#else
                std::string launcherName = "mtype-launcher";
#endif
                std::string launcherPath;
                std::vector<std::string> searchPaths = {
                    "bin/mType/Debug/x64/" + launcherName,
                    "bin/mType/Release/x64/" + launcherName,
                    "bin/mtype-launcher/Debug/x64/" + launcherName,
                    "bin/mtype-launcher/Release/x64/" + launcherName
                };
                for (const auto& candidate : searchPaths) {
                    if (fs::exists(candidate)) {
                        launcherPath = candidate;
                        break;
                    }
                }
                if (launcherPath.empty()) {
                    // Skip exe test if launcher not built
                    throw std::runtime_error("SKIP: mtype-launcher not found (build it first)");
                }

                // === Step 4: Build consumer exe ===
                std::string consumerMtproj = "mType/tests/testFiles/library/projects/consumer/Consumer.mtproj";
                auto consumerConfig = configParser.parse(consumerMtproj);
                require(consumerConfig != nullptr, "Failed to parse Consumer.mtproj");

                fs::path consumerOutputDir = fs::path(consumerConfig->projectRoot) / consumerConfig->output.directory;
#ifdef _WIN32
                std::string exeName = consumerConfig->name + ".exe";
#else
                std::string exeName = consumerConfig->name;
#endif
                std::string exePath = (consumerOutputDir / exeName).string();

                project::ProjectBuilder consumerBuilder;
                auto consumerResult = consumerBuilder.buildExecutable(
                    *consumerConfig, exePath, launcherPath);
                require(consumerResult.success, "Consumer exe build failed: " +
                    (consumerResult.errors.empty() ? "unknown" : consumerResult.errors[0]));
                require(fs::exists(exePath), "Consumer exe not created");

                // Verify libs/ was copied next to exe
                fs::path exeLibsDir = consumerOutputDir / "libs";
                require(fs::exists(exeLibsDir / "MathLib.mtcLib"),
                    "MathLib.mtcLib should be copied to exe's libs/ directory");

                // === Step 5: Run the exe and capture output ===
                std::string command = "\"" + exePath + "\" 2>&1";
#ifdef _WIN32
                FILE* pipe = _popen(command.c_str(), "r");
#else
                FILE* pipe = popen(command.c_str(), "r");
#endif
                require(pipe != nullptr, "Failed to run consumer exe");

                std::string output;
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    output += buffer;
                }
#ifdef _WIN32
                int exitCode = _pclose(pipe);
#else
                int exitCode = pclose(pipe);
#endif

                // === Step 6: Verify output ===
                std::string expectedPath = "mType/tests/testFiles/library/projects/consumer/Consumer.expected";
                require(fs::exists(expectedPath), "Consumer.expected not found");

                std::ifstream expectedFile(expectedPath);
                std::string expected((std::istreambuf_iterator<char>(expectedFile)),
                                     std::istreambuf_iterator<char>());
                expectedFile.close();

                // Normalize line endings
                auto normalize = [](std::string s) {
                    std::string result;
                    for (char c : s) {
                        if (c != '\r') result += c;
                    }
                    while (!result.empty() && (result.back() == '\n' || result.back() == ' ')) {
                        result.pop_back();
                    }
                    return result;
                };

                std::string normOutput = normalize(output);
                std::string normExpected = normalize(expected);

                require(normOutput == normExpected,
                    "Output mismatch.\n\nExpected:\n" + normExpected +
                    "\n\nActual:\n" + normOutput);

                require(exitCode == 0,
                    "Consumer exe exited with code " + std::to_string(exitCode));

                // === Cleanup ===
                try {
                    fs::remove(destLib);
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                    fs::remove_all(consumerOutputDir);
                } catch (...) {}
            });
    }

    // =========================================================================
    // MYT-100: Transitive dependency loading at runtime
    // =========================================================================

    void LibraryTestSuite::setupTransitiveDependencyTests()
    {
        addCallbackTest("TransitiveDependencyLoader loads single library with no deps",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(libMtproj), "MathLib.mtproj not found");

                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));

                // Load via TransitiveDependencyLoader
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                loader.loadLibraryWithDependencies(libPath, *vm, env);

                // Verify symbols were registered
                require(env->isLibraryLoaded("MathLib"), "MathLib should be marked as loaded");
                require(env->findClass("MathUtils") != nullptr, "MathUtils should be registered");
                require(env->findClass("Vector2") != nullptr, "Vector2 should be registered");

                // Cleanup
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("TransitiveDependencyLoader skips already-loaded libraries",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                // Load twice — second should be a no-op
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                loader.loadLibraryWithDependencies(libPath, *vm, env);
                size_t countAfterFirst = vm->getLoadedProgramCount();

                loader.loadLibraryWithDependencies(libPath, *vm, env);
                size_t countAfterSecond = vm->getLoadedProgramCount();

                require(countAfterFirst == countAfterSecond,
                    "Loading same library twice should not add duplicate programs");

                // Cleanup
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("TransitiveDependencyLoader batch loads multiple libraries",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                // Load via batch method
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                std::vector<std::string> paths = { libPath };
                loader.loadLibrariesWithDependencies(paths, *vm, env);

                require(env->isLibraryLoaded("MathLib"), "MathLib should be loaded via batch");
                require(env->findClass("MathUtils") != nullptr, "MathUtils should be registered");

                // Cleanup
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("TransitiveDependencyLoader throws on missing file",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                bool threw = false;
                try {
                    loader.loadLibraryWithDependencies("nonexistent/path/Fake.mtcLib", *vm, env);
                } catch (const std::runtime_error& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("Could not open") != std::string::npos,
                        "Error should mention file open failure, got: " + msg);
                }
                require(threw, "Should throw on missing file");
            });
    }

    // =========================================================================
    // MYT-101: Native loadLibrary API tests
    // =========================================================================

    void LibraryTestSuite::setupNativeLoadLibraryTests()
    {
        addCallbackTest("LibraryNatives validates empty path",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                // Call the native function with empty path
                auto nativeRegistry = env->getNativeRegistry();
                require(nativeRegistry->hasNativeFunction("loadLibrary"),
                    "loadLibrary should be registered as a native function");

                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");
                require(static_cast<bool>(loadLibFunc), "loadLibrary function should be found");

                bool threw = false;
                try {
                    invokeNative(loadLibFunc, env, vm, { std::string("") });
                } catch (const errors::RuntimeException& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("empty") != std::string::npos,
                        "Error should mention empty path, got: " + msg);
                }
                require(threw, "Should throw on empty path");

                LibraryNatives::cleanup();
            });

        addCallbackTest("LibraryNatives validates nonexistent file",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");

                bool threw = false;
                try {
                    invokeNative(loadLibFunc, env, vm, { std::string("nonexistent/Fake.mtcLib") });
                } catch (const errors::RuntimeException& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("not found") != std::string::npos,
                        "Error should mention file not found, got: " + msg);
                }
                require(threw, "Should throw on nonexistent file");

                LibraryNatives::cleanup();
            });

        addCallbackTest("LibraryNatives validates wrong extension",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Create a temporary file with wrong extension
                std::string tempPath = "mType/tests/testFiles/library/libs/test_wrong_ext.txt";
                fs::create_directories("mType/tests/testFiles/library/libs");
                {
                    std::ofstream out(tempPath);
                    out << "not a library";
                }

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");

                bool threw = false;
                try {
                    invokeNative(loadLibFunc, env, vm, { std::string(tempPath) });
                } catch (const errors::RuntimeException& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find(".mtcLib") != std::string::npos,
                        "Error should mention .mtcLib extension, got: " + msg);
                }
                require(threw, "Should throw on wrong extension");

                // Cleanup
                LibraryNatives::cleanup();
                try { fs::remove(tempPath); } catch (...) {}
            });

        addCallbackTest("LibraryNatives validates wrong argument count",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");

                bool threw = false;
                try {
                    invokeNative(loadLibFunc, env, vm, {});  // No arguments
                } catch (const errors::RuntimeException& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("1 argument") != std::string::npos,
                        "Error should mention argument count, got: " + msg);
                }
                require(threw, "Should throw on wrong argument count");

                LibraryNatives::cleanup();
            });

        addCallbackTest("LibraryNatives validates non-string argument",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");

                bool threw = false;
                try {
                    invokeNative(loadLibFunc, env, vm, { int64_t(42) });  // Integer instead of string
                } catch (const errors::RuntimeException& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("string") != std::string::npos,
                        "Error should mention string type, got: " + msg);
                }
                require(threw, "Should throw on non-string argument");

                LibraryNatives::cleanup();
            });

        addCallbackTest("LibraryNatives loads valid .mtcLib via native function",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(libMtproj), "MathLib.mtproj not found");

                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                // Create fresh environment and call native function
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");

                // Load library via native function
                auto result = invokeNative(loadLibFunc, env, vm, { std::string(libPath) });
                require(value::isVoid(result),
                    "loadLibrary should return void");

                // Verify library was loaded
                require(env->isLibraryLoaded("MathLib"), "MathLib should be loaded");
                require(env->findClass("MathUtils") != nullptr, "MathUtils should be registered");
                require(env->findClass("Vector2") != nullptr, "Vector2 should be registered");

                // Cleanup
                LibraryNatives::cleanup();
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });
    }

    // =========================================================================
    // Unload library tests
    // =========================================================================

    void LibraryTestSuite::setupUnloadLibraryTests()
    {
        addCallbackTest("unloadLibrary removes symbols and unmarks library",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                // Load it
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                loader.loadLibraryWithDependencies(libPath, *vm, env);

                require(env->isLibraryLoaded("MathLib"), "MathLib should be loaded");
                require(env->findClass("MathUtils") != nullptr, "MathUtils should exist");
                require(env->findClass("Vector2") != nullptr, "Vector2 should exist");

                // Unload it
                loader.unloadLibrary("MathLib", *vm, env);

                require(!env->isLibraryLoaded("MathLib"), "MathLib should no longer be loaded");
                require(env->findClass("MathUtils") == nullptr, "MathUtils should be removed");
                require(env->findClass("Vector2") == nullptr, "Vector2 should be removed");

                // Cleanup
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("unloadLibrary throws on unknown library",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                bool threw = false;
                try {
                    loader.unloadLibrary("NonExistent", *vm, env);
                } catch (const std::runtime_error& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("not loaded") != std::string::npos,
                        "Error should mention not loaded, got: " + msg);
                }
                require(threw, "Should throw on unknown library");
            });

        addCallbackTest("unloadLibrary via native function",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                // Create fresh environment and load via native function
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto loadLibFunc = nativeRegistry->findNativeFunction("loadLibrary");
                auto unloadLibFunc = nativeRegistry->findNativeFunction("unloadLibrary");

                require(static_cast<bool>(unloadLibFunc), "unloadLibrary should be registered");

                // Load then unload
                invokeNative(loadLibFunc, env, vm, { std::string(libPath) });
                require(env->isLibraryLoaded("MathLib"), "MathLib should be loaded");

                invokeNative(unloadLibFunc, env, vm, { std::string("MathLib") });
                require(!env->isLibraryLoaded("MathLib"), "MathLib should be unloaded");
                require(env->findClass("MathUtils") == nullptr, "MathUtils should be gone");

                // Cleanup
                LibraryNatives::cleanup();
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });

        addCallbackTest("unloadLibrary native throws on not-loaded library",
            "",
            [](ScriptAPI&) {
                using namespace project::mtclib;

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                // Re-register library natives with a loader the test owns.
                // EnvironmentBuilder's loader is local to build() and is destroyed on return,
                // leaving the userData raw pointer dangling — would crash on first dereference.
                auto loader = std::make_shared<TransitiveDependencyLoader>();
                LibraryNatives::registerAll(env, loader);

                auto nativeRegistry = env->getNativeRegistry();
                auto unloadLibFunc = nativeRegistry->findNativeFunction("unloadLibrary");

                bool threw = false;
                try {
                    invokeNative(unloadLibFunc, env, vm, { std::string("NotLoaded") });
                } catch (const errors::RuntimeException& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("not loaded") != std::string::npos,
                        "Error should mention not loaded, got: " + msg);
                }
                require(threw, "Should throw on not-loaded library");

                LibraryNatives::cleanup();
            });

        addCallbackTest("unloadLibrary blocks when another loaded library depends on it",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Step 1: Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                // Step 2: Copy MathLib to consumer/libs/ and build Consumer
                std::string consumerLibsDir = "mType/tests/testFiles/library/projects/consumer/libs";
                fs::create_directories(consumerLibsDir);
                std::string destLib = consumerLibsDir + "/MathLib.mtcLib";
                fs::copy_file(libPath, destLib, fs::copy_options::overwrite_existing);

                std::string consumerMtproj = "mType/tests/testFiles/library/projects/consumer/Consumer.mtproj";
                auto consumerConfig = configParser.parse(consumerMtproj);
                require(consumerConfig != nullptr, "Failed to parse Consumer.mtproj");

                fs::path consumerOutputDir = fs::path(consumerConfig->projectRoot) / consumerConfig->output.directory;
                fs::create_directories(consumerOutputDir);
                std::string consumerLibPath = (consumerOutputDir / (consumerConfig->name + ".mtcLib")).string();

                // Remove stale output to defeat incremental build cache
                try { fs::remove(consumerLibPath); } catch (...) {}

                project::ProjectBuilder consumerBuilder;
                auto consumerResult = consumerBuilder.buildLibrary(*consumerConfig, consumerLibPath);
                require(consumerResult.success, "Consumer build failed: " +
                    (consumerResult.errors.empty() ? "unknown" : consumerResult.errors[0]));

                // Step 3: Load both via transitive loader
                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;
                loader.addSearchPath(libOutputDir.string());
                loader.addSearchPath(consumerLibsDir);

                loader.loadLibraryWithDependencies(consumerLibPath, *vm, env);

                require(env->isLibraryLoaded("MathLib"), "MathLib should be loaded transitively");
                require(env->isLibraryLoaded("Consumer"), "Consumer should be loaded");

                // Step 4: Try to unload MathLib — should fail because Consumer depends on it
                bool threw = false;
                try {
                    loader.unloadLibrary("MathLib", *vm, env);
                } catch (const std::runtime_error& e) {
                    threw = true;
                    std::string msg = e.what();
                    require(msg.find("Consumer") != std::string::npos,
                        "Error should name the dependent library, got: " + msg);
                }
                require(threw, "Should refuse to unload MathLib while Consumer depends on it");

                // Verify MathLib is still loaded
                require(env->isLibraryLoaded("MathLib"), "MathLib should still be loaded after failed unload");

                // Cleanup
                try {
                    fs::remove(destLib);
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                    fs::remove(consumerLibPath);
                    fs::remove_all(consumerOutputDir);
                } catch (...) {}
            });

        addCallbackTest("unloadLibrary allows re-load after unload",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;
                using namespace project::mtclib;

                // Build MathLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                project::ProjectConfigParser configParser;
                auto libConfig = configParser.parse(libMtproj);

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                fs::create_directories(libOutputDir);
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed");

                environment::EnvironmentBuilder envBuilder;
                auto env = envBuilder.build();
                auto vm = std::make_shared<vm::runtime::VirtualMachine>(env);

                TransitiveDependencyLoader loader;

                // Load -> unload -> reload
                loader.loadLibraryWithDependencies(libPath, *vm, env);
                require(env->isLibraryLoaded("MathLib"), "Should be loaded");

                loader.unloadLibrary("MathLib", *vm, env);
                require(!env->isLibraryLoaded("MathLib"), "Should be unloaded");
                require(env->findClass("MathUtils") == nullptr, "MathUtils should be gone");

                loader.loadLibraryWithDependencies(libPath, *vm, env);
                require(env->isLibraryLoaded("MathLib"), "Should be re-loaded");
                require(env->findClass("MathUtils") != nullptr, "MathUtils should be back");

                // Cleanup
                try {
                    fs::remove(libPath);
                    fs::remove_all(libOutputDir);
                } catch (...) {}
            });
    }

    // =========================================================================
    // MYT-77: Class tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupClassTests()
    {
        addOutputVerificationTest("Library class basic definition", passPath + "classBasicDefinition.mt");
        addTestFromFile("Library class access private error", errorPath + "classAccessPrivate.mt",
            TestType::ERROR_EXPECTED);
    }

    // =========================================================================
    // MYT-78: Array tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupArrayTests()
    {
        addOutputVerificationTest("Library array return type", passPath + "arrayReturnType.mt");
    }

    // =========================================================================
    // MYT-79: Interface tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupInterfaceTests()
    {
        addOutputVerificationTest("Library interface implementation", passPath + "interfaceImplementation.mt");
    }

    // =========================================================================
    // MYT-80: Lambda tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupLambdaTests()
    {
        addOutputVerificationTest("Library functional interface lambda", passPath + "lambdaFunctionalInterface.mt");
    }

    // =========================================================================
    // MYT-81: Async tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupAsyncTests()
    {
        addOutputVerificationTest("Library async function", passPath + "asyncLibraryFunction.mt");
    }

    // =========================================================================
    // MYT-82: Casting tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupCastTests()
    {
        addOutputVerificationTest("Library instanceof check", passPath + "castInstanceof.mt");
    }

    // =========================================================================
    // MYT-83: Import tests with mtcLib
    // =========================================================================

    void LibraryTestSuite::setupImportTests()
    {
        addOutputVerificationTest("Import lib syntax parse", passPath + "importLibSyntax.mt");
        addTestFromFile("Import lib missing library error", errorPath + "importLibMissing.mt",
            TestType::ERROR_EXPECTED);
    }

    // =========================================================================
    // MYT-84: Collection tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupCollectionTests()
    {
        addOutputVerificationTest("Library collection usage", passPath + "collectionUsage.mt");
    }

    // =========================================================================
    // MYT-85: Reflection tests on library types
    // =========================================================================

    void LibraryTestSuite::setupReflectionTests()
    {
        addOutputVerificationTest("Library type reflection", passPath + "reflectionLibraryType.mt");
    }

    // =========================================================================
    // MYT-86: Value class tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupValueClassTests()
    {
        addOutputVerificationTest("Library value class copy semantics", passPath + "valueClassCopy.mt");
    }

    // =========================================================================
    // MYT-87: For-each tests with library types
    // =========================================================================

    void LibraryTestSuite::setupForEachTests()
    {
        addOutputVerificationTest("Library for-each iteration", passPath + "forEachLibrary.mt");
    }

    // =========================================================================
    // MYT-88: Try-catch tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupTryCatchTests()
    {
        addOutputVerificationTest("Library exception handling", passPath + "tryCatchLibrary.mt");
    }

    // =========================================================================
    // MYT-89: Generics tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupGenericsTests()
    {
        addOutputVerificationTest("Library generic class usage", passPath + "genericsLibraryClass.mt");
    }

    // =========================================================================
    // MYT-90: Static method tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupStaticTests()
    {
        addOutputVerificationTest("Library static method call", passPath + "staticMethodCall.mt");
    }

    // =========================================================================
    // MYT-91: Inheritance tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupInheritanceTests()
    {
        addOutputVerificationTest("Library class inheritance", passPath + "inheritanceExtendLibrary.mt");
        addTestFromFile("Library final class extend error", errorPath + "inheritanceFinalClass.mt",
            TestType::ERROR_EXPECTED);
    }

    // =========================================================================
    // MYT-92: Access modifier tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupModifierTests()
    {
        addOutputVerificationTest("Library access modifiers", passPath + "modifierAccessControl.mt");
    }

    // =========================================================================
    // MYT-93: Stream API tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupStreamTests()
    {
        addOutputVerificationTest("Library stream operations", passPath + "streamLibraryOps.mt");
    }

    // =========================================================================
    // MYT-94: Pattern matching tests across library boundaries
    // =========================================================================

    void LibraryTestSuite::setupPatternMatchingTests()
    {
        addOutputVerificationTest("Library pattern matching", passPath + "patternMatchLibrary.mt");
    }
}
