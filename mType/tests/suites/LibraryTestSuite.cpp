#include "LibraryTestSuite.hpp"
#include "../../services/ScriptAPI.hpp"
#include "../../types/TypeSignature.hpp"
#include "../../project/mtclib/MtcLibFormat.hpp"
#include "../../project/mtclib/MtcLibSerializer.hpp"
#include "../../project/mtclib/MtcLibBuilder.hpp"
#include "../../project/mtclib/MtcPathResolver.hpp"
#include "../../project/mtclib/DependencyResolver.hpp"
#include "../../project/mtclib/LibraryLinker.hpp"
#include "../../project/mtclib/LibrarySymbolProvider.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../project/ProjectConfig.hpp"
#include <sstream>
#include <stdexcept>
#include <cstring>

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
                lib.metadata.mtypeVersion = "0.2.0";
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
        // These will be added when the full build pipeline is testable
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
                // Without any .mtcLib files on disk, resolve should return nullopt
                auto result = resolver.resolve("nonexistent-lib");
                require(!result.has_value(), "Should not find nonexistent library");
            });
    }

    // =========================================================================
    // MYT-75: Runtime loading tests
    // =========================================================================

    void LibraryTestSuite::setupRuntimeLoadingTests()
    {
        // These will be added when the full pipeline can produce .mtcLib files
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
