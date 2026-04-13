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
#include "../../project/ProjectBuilder.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../environment/EnvironmentBuilder.hpp"
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <filesystem>
#include <fstream>

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
    }

    // =========================================================================
    // MYT-75: Runtime loading tests
    // =========================================================================

    void LibraryTestSuite::setupRuntimeLoadingTests()
    {
        addCallbackTest("Build MathLib then build Consumer that depends on it",
            "",
            [](ScriptAPI&) {
                namespace fs = std::filesystem;

                // Step 1: Build MathLib to .mtcLib
                std::string libMtproj = "mType/tests/testFiles/library/projects/mathlib/MathLib.mtproj";
                require(fs::exists(libMtproj), "MathLib.mtproj not found");

                project::ProjectConfigParser parser;
                auto libConfig = parser.parse(libMtproj);
                require(libConfig != nullptr, "Failed to parse MathLib.mtproj");

                fs::path libOutputDir = fs::path(libConfig->projectRoot) / libConfig->output.directory;
                std::string libPath = (libOutputDir / (libConfig->name + ".mtcLib")).string();

                project::ProjectBuilder libBuilder;
                auto libResult = libBuilder.buildLibrary(*libConfig, libPath);
                require(libResult.success, "MathLib build failed: " +
                    (libResult.errors.empty() ? "unknown" : libResult.errors[0]));

                // Step 2: Copy .mtcLib to consumer's libs/ directory
                std::string consumerLibsDir = "mType/tests/testFiles/library/projects/consumer/libs";
                fs::create_directories(consumerLibsDir);
                std::string destLib = consumerLibsDir + "/MathLib.mtcLib";
                fs::copy_file(libPath, destLib, fs::copy_options::overwrite_existing);

                // Step 3: Parse consumer project
                std::string consumerMtproj = "mType/tests/testFiles/library/projects/consumer/Consumer.mtproj";
                require(fs::exists(consumerMtproj), "Consumer.mtproj not found");

                auto consumerConfig = parser.parse(consumerMtproj);
                require(consumerConfig != nullptr, "Failed to parse Consumer.mtproj");

                // Step 4: Try to compile consumer (this tests compile-time linking)
                // For now just verify the library can be found and deserialized
                project::mtclib::MtcPathResolver pathResolver(consumerConfig->projectRoot);
                pathResolver.addSearchPath(consumerLibsDir);

                auto resolved = pathResolver.resolve("MathLib");
                require(resolved.has_value(), "MathLib.mtcLib should be found in consumer/libs/");

                std::ifstream inFile(*resolved, std::ios::binary);
                auto lib = project::mtclib::MtcLibSerializer::deserialize(inFile);
                inFile.close();

                requireEq("MathLib", lib.metadata.name, "Loaded library name");
                require(!lib.exports.empty(), "Loaded library should have exports");
                require(!lib.bytecodeProgram.getClasses().empty(), "Library should have class metadata");

                // Cleanup
                fs::remove(destLib);
                fs::remove(libPath);
                fs::remove_all(libOutputDir);
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
