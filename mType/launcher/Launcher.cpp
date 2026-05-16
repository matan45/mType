/**
 * Launcher.cpp - Standalone executable launcher for mType bytecode
 *
 * The mType build system appends a serialized BytecodeProgram blob after
 * this native binary, followed by an 8-byte little-endian blob size and
 * a 4-byte magic footer "MTEX". At startup the launcher reads the
 * appended blob, deserializes it, finds the @EntryPoint class, and
 * invokes its static main(String[] args): void method.
 *
 * Binary layout:
 *   [ native launcher binary ]
 *   [ serialized BytecodeProgram blob (N bytes) ]
 *   [ uint64_t N (blob size, 8 bytes, little-endian) ]
 *   [ 4-byte magic: "MTEX" ]
 */

#include "../services/ScriptInterpreter.hpp"
#include <cstddef>
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../value/NativeArray.hpp"
#include "../gc/GC.hpp"
#include "../plugin/PluginLoader.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <climits>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#include <bit>
static_assert(std::endian::native == std::endian::little,
              "MTEX binary format assumes little-endian byte order for the blob size field");

static const char FOOTER_MAGIC[4] = { 'M', 'T', 'E', 'X' };

static std::string getExecutablePath()
{
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        throw std::runtime_error("Failed to determine executable path");
    }
    return std::string(buf, len);
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0)
    {
        throw std::runtime_error("Failed to determine executable path");
    }
    return std::string(buf);
#else
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len <= 0)
    {
        throw std::runtime_error("Failed to determine executable path");
    }
    buf[len] = '\0';
    return std::string(buf);
#endif
}

static std::vector<char> readAppendedBytecode(const std::string& exePath)
{
    std::ifstream file(exePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        throw std::runtime_error("Could not open executable: " + exePath);
    }

    auto fileSize = file.tellg();
    if (fileSize < 12)
    {
        throw std::runtime_error("Executable too small to contain embedded bytecode");
    }

    // Read 4-byte magic at end
    file.seekg(-4, std::ios::end);
    char magic[4];
    file.read(magic, 4);
    if (file.fail() || std::memcmp(magic, FOOTER_MAGIC, 4) != 0)
    {
        throw std::runtime_error("No embedded bytecode found (missing MTEX footer)");
    }

    // Read 8-byte blob size before magic
    file.seekg(-12, std::ios::end);
    uint64_t blobSize = 0;
    file.read(reinterpret_cast<char*>(&blobSize), sizeof(blobSize));
    if (file.fail())
    {
        throw std::runtime_error("Failed to read embedded bytecode size");
    }

    auto footerEnd = static_cast<uint64_t>(fileSize);
    if (blobSize == 0 || blobSize > footerEnd - 12)
    {
        throw std::runtime_error("Invalid embedded bytecode size");
    }

    // Read the blob
    std::vector<char> blob(blobSize);
    file.seekg(-12 - static_cast<std::streamoff>(blobSize), std::ios::end);
    file.read(blob.data(), static_cast<std::streamsize>(blobSize));
    if (file.fail())
    {
        throw std::runtime_error("Failed to read embedded bytecode blob");
    }

    return blob;
}

static void loadSidecarLibraries(
    const std::string& exePath,
    services::ScriptInterpreter& interpreter)
{
    namespace fs = std::filesystem;

    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path libsDir = exeDir / "libs";

    if (!fs::exists(libsDir) || !fs::is_directory(libsDir)) {
        return;  // No libs directory — nothing to load
    }

    // Add exe directory as a search path for transitive dependency resolution
    interpreter.addLibrarySearchPath(exeDir.string());

    // Collect and sort library paths for deterministic discovery across platforms
    std::vector<std::string> libs;
    for (const auto& entry : fs::directory_iterator(libsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mtcLib") {
            libs.push_back(entry.path().string());
        }
    }
    std::sort(libs.begin(), libs.end());

    // Load all libraries with transitive dependency resolution
    interpreter.loadLibrariesWithDependencies(libs);
}

static std::string findEntryPointClass(
    std::shared_ptr<environment::Environment> env)
{
    auto classRegistry = env->getClassRegistry();
    std::vector<std::string> entryPoints;
    for (const auto& name : classRegistry->getAllItemNames())
    {
        auto classDef = classRegistry->findClass(name);
        if (classDef && classDef->getAnnotation("EntryPoint"))
        {
            entryPoints.push_back(name);
        }
    }

    if (entryPoints.empty())
    {
        throw std::runtime_error("No class with @EntryPoint annotation found in bytecode");
    }
    if (entryPoints.size() > 1)
    {
        std::string msg = "Multiple @EntryPoint classes found:";
        for (const auto& name : entryPoints)
        {
            msg += " " + name;
        }
        msg += ". Only one @EntryPoint class is allowed per executable.";
        throw std::runtime_error(msg);
    }
    return entryPoints[0];
}

int main(int argc, char* argv[])
{
    try
    {
        // Read appended bytecode
        std::string exePath = getExecutablePath();
        auto blob = readAppendedBytecode(exePath);

        // Register exeDir as a plugin search root so user code that loads a
        // plugin via a project-relative path (e.g. "mt_modules/.../foo.dll")
        // still resolves when CWD differs from the original project root.
        ::plugin::PluginLoader::instance().addSearchPath(
            std::filesystem::path(exePath).parent_path().string());

        // Deserialize (wrap vector in a streambuf to avoid a second copy)
        struct VectorBuf : std::streambuf {
            VectorBuf(std::vector<char>& v) { setg(v.data(), v.data(), v.data() + v.size()); }
        } buf(blob);
        std::istream stream(&buf);
        auto program = vm::bytecode::BytecodeProgram::deserialize(stream);

        // Boot interpreter: load main program (registers classes, does NOT execute yet)
        services::ScriptInterpreter interpreter;
        interpreter.loadFromProgram(std::move(program));

        // Load .mtcLib files from libs/ directory next to the exe
        // (must happen after main program is set so loadedPrograms[0] = main)
        loadSidecarLibraries(exePath, interpreter);

        // Run static field initializers before invoking the entry point.
        interpreter.runStaticInitializersForLoadedPrograms();

        // Find @EntryPoint class
        auto env = interpreter.getEnvironment();
        std::string entryClass = findEntryPointClass(env);

        // Build string[] args from argv (skip argv[0] which is the exe name)
        size_t argCount = (argc > 1) ? static_cast<size_t>(argc - 1) : 0;
        auto argsArray = std::make_shared<value::NativeArray>(argCount, value::ValueType::STRING);
        for (size_t i = 0; i < argCount; ++i)
        {
            argsArray->set(i, value::Value(std::string(argv[i + 1])));
        }

        // Call static main(args)
        value::Value argsValue = argsArray;
        interpreter.callStaticMethod(entryClass, "main", { argsValue });

        // Run GC cleanup
        if (gc::GC::isInitialized())
        {
            gc::GC::get()->forceCollect();
        }
        gc::GC::shutdown();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
