#include "PackageCompiler.hpp"
#include "PackageManifest.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#endif

namespace packagemanager
{
    namespace fs = std::filesystem;

    PackageCompiler::PackageCompiler() = default;

    void PackageCompiler::setMTypeExecutable(const std::string& path)
    {
        mTypeExePath = path;
    }

    void PackageCompiler::setProgressCallback(CompileProgressCallback callback)
    {
        progressCallback = std::move(callback);
    }

    void PackageCompiler::reportProgress(const std::string& message)
    {
        if (progressCallback)
        {
            progressCallback(message);
        }
    }

    std::string PackageCompiler::discoverMTypeExecutable(const std::string& mtpmExePath)
    {
#ifdef _WIN32
        const std::string exeName = "mType.exe";
#else
        const std::string exeName = "mType";
#endif
        if (mtpmExePath.empty())
        {
            return "";
        }

        fs::path mtpmPath(mtpmExePath);
        std::error_code ec;

        // 1. Sibling layout (production install: both binaries in one dir).
        fs::path sibling = mtpmPath.parent_path() / exeName;
        if (fs::exists(sibling, ec))
        {
            return sibling.string();
        }

        // 2. Parallel bin layout (dev build):
        //   bin/mtpm/<cfg>/<plat>/mtpm.exe ↔ bin/mType/<cfg>/<plat>/mType.exe
        fs::path mtpmDir = mtpmPath.parent_path();
        if (mtpmDir.has_parent_path() &&
            mtpmDir.parent_path().has_parent_path() &&
            mtpmDir.parent_path().parent_path().filename() == "mtpm")
        {
            fs::path binRoot = mtpmDir.parent_path().parent_path().parent_path();
            fs::path candidate = binRoot / "mType" /
                                 mtpmDir.parent_path().filename() /
                                 mtpmDir.filename() / exeName;
            if (fs::exists(candidate, ec))
            {
                return candidate.string();
            }
        }

        return "";
    }

    bool PackageCompiler::isUpToDate(const ResolvedPackage& pkg,
                                     const std::string& sourceDir) const
    {
        fs::path libPath = fs::path(pkg.registryPath) / (pkg.name + ".mtcLib");
        std::error_code ec;
        if (!fs::exists(libPath, ec))
        {
            return false;
        }

        auto libTime = fs::last_write_time(libPath, ec);
        if (ec) return false;

        fs::path scanRoot = fs::path(pkg.registryPath) / sourceDir;
        if (!fs::exists(scanRoot, ec))
        {
            scanRoot = fs::path(pkg.registryPath);
        }

        for (const auto& entry : fs::recursive_directory_iterator(scanRoot, ec))
        {
            if (ec) return false;
            if (!entry.is_regular_file(ec)) continue;
            auto ext = entry.path().extension().string();
            if (ext != ".mt" && ext != ".json") continue;
            auto fileTime = fs::last_write_time(entry.path(), ec);
            if (ec) return false;
            if (fileTime > libTime) return false;
        }
        return true;
    }

    std::vector<std::string> PackageCompiler::topologicalOrder(
        const std::unordered_map<std::string, ResolvedPackage>& resolved)
    {
        // Build in-degrees only over edges that point to packages also present
        // in `resolved` — external/system deps are ignored for ordering.
        std::unordered_map<std::string, size_t> inDegree;
        std::unordered_map<std::string, std::vector<std::string>> dependents;

        for (const auto& [name, _] : resolved)
        {
            inDegree[name] = 0;
        }

        for (const auto& [name, pkg] : resolved)
        {
            for (const auto& [depName, _ver] : pkg.dependencies)
            {
                if (resolved.find(depName) == resolved.end()) continue;
                dependents[depName].push_back(name);
                inDegree[name]++;
            }
        }

        std::vector<std::string> order;
        std::vector<std::string> ready;
        ready.reserve(resolved.size());
        for (const auto& [name, deg] : inDegree)
        {
            if (deg == 0) ready.push_back(name);
        }
        // Deterministic emission order
        std::sort(ready.begin(), ready.end());

        while (!ready.empty())
        {
            std::string current = ready.back();
            ready.pop_back();
            order.push_back(current);

            auto it = dependents.find(current);
            if (it == dependents.end()) continue;
            std::vector<std::string> newlyReady;
            for (const auto& d : it->second)
            {
                if (--inDegree[d] == 0)
                {
                    newlyReady.push_back(d);
                }
            }
            std::sort(newlyReady.begin(), newlyReady.end());
            for (auto& d : newlyReady) ready.push_back(d);
        }

        // If a cycle exists, append remaining nodes (best effort — compilation
        // will surface the actual semantic problem).
        if (order.size() != resolved.size())
        {
            std::unordered_set<std::string> seen(order.begin(), order.end());
            for (const auto& [name, _] : resolved)
            {
                if (!seen.count(name)) order.push_back(name);
            }
        }
        return order;
    }

    static std::string escapeXmlAttr(const std::string& str)
    {
        std::string out;
        out.reserve(str.size());
        for (char c : str)
        {
            switch (c)
            {
                case '&':  out += "&amp;";  break;
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&apos;"; break;
                default:   out += c;        break;
            }
        }
        return out;
    }

    std::string PackageCompiler::buildSyntheticMtproj(
        const ResolvedPackage& pkg,
        const std::string& sourceInclude,
        const std::unordered_map<std::string, ResolvedPackage>& resolved)
    {
        std::ostringstream xml;
        xml << "<Project Name=\"" << escapeXmlAttr(pkg.name)
            << "\" Version=\"" << escapeXmlAttr(pkg.version) << "\">\n";
        xml << "  <Source>\n";
        xml << "    <Include>" << escapeXmlAttr(sourceInclude) << "</Include>\n";
        xml << "  </Source>\n";
        xml << "  <Output Directory=\".\" />\n";
        if (!pkg.dependencies.empty())
        {
            xml << "  <Dependencies>\n";
            for (const auto& [depName, _depRange] : pkg.dependencies)
            {
                // Use the resolved exact version, not the range from mtpkg.json —
                // matches what's actually on disk in the registry.
                auto it = resolved.find(depName);
                std::string exactVersion = (it != resolved.end()) ? it->second.version : _depRange;
                xml << "    <Package Name=\"" << escapeXmlAttr(depName)
                    << "\" Version=\"" << escapeXmlAttr(exactVersion) << "\" />\n";
            }
            xml << "  </Dependencies>\n";
        }
        xml << "</Project>\n";
        return xml.str();
    }

#ifdef _WIN32
    int PackageCompiler::runMTypeBuild(const std::string& mtprojPath, std::string& output) const
    {
        std::string exe = mTypeExePath.empty() ? "mType.exe" : mTypeExePath;

        // Quote args; the command line is interpreted by CommandLineToArgvW.
        std::string cmdLine = "\"" + exe + "\" --build --lib \"" + mtprojPath + "\"";

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE readPipe = nullptr;
        HANDLE writePipe = nullptr;
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        {
            return -1;
        }
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError = writePipe;

        PROCESS_INFORMATION pi{};

        std::vector<char> mutableCmd(cmdLine.begin(), cmdLine.end());
        mutableCmd.push_back('\0');

        BOOL ok = CreateProcessA(
            nullptr,
            mutableCmd.data(),
            nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW,
            nullptr, nullptr,
            &si, &pi);

        CloseHandle(writePipe);

        if (!ok)
        {
            CloseHandle(readPipe);
            return -1;
        }

        char buffer[512];
        DWORD bytesRead = 0;
        while (ReadFile(readPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        CloseHandle(readPipe);

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return static_cast<int>(exitCode);
    }
#else
    int PackageCompiler::runMTypeBuild(const std::string& mtprojPath, std::string& output) const
    {
        std::string exe = mTypeExePath.empty() ? "mType" : mTypeExePath;

        int pipefd[2];
        if (pipe(pipefd) != 0)
        {
            return -1;
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }

        if (pid == 0)
        {
            // Child
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            std::vector<const char*> argv;
            argv.push_back(exe.c_str());
            argv.push_back("--build");
            argv.push_back("--lib");
            argv.push_back(mtprojPath.c_str());
            argv.push_back(nullptr);

            execvp(exe.c_str(), const_cast<char* const*>(argv.data()));
            _exit(127);
        }

        close(pipefd[1]);
        char buffer[512];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[n] = '\0';
            output += buffer;
        }
        close(pipefd[0]);

        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
#endif

    bool PackageCompiler::compileOne(
        const ResolvedPackage& pkg,
        const std::unordered_map<std::string, ResolvedPackage>& resolved,
        std::string& error)
    {
        std::error_code ec;
        fs::path manifestPath = fs::path(pkg.registryPath) / "mtpkg.json";
        if (!fs::exists(manifestPath, ec))
        {
            error = "no mtpkg.json at " + manifestPath.string();
            return false;
        }

        std::string manifestJson;
        try
        {
            std::ifstream in(manifestPath);
            std::ostringstream buf;
            buf << in.rdbuf();
            manifestJson = buf.str();
        }
        catch (const std::exception& e)
        {
            error = std::string("read mtpkg.json failed: ") + e.what();
            return false;
        }

        PackageManifest manifest;
        try
        {
            manifest = PackageManifest::parseFromJson(manifestJson);
        }
        catch (const std::exception& e)
        {
            error = std::string("parse mtpkg.json failed: ") + e.what();
            return false;
        }

        std::string sourceDir = manifest.sourceDir.empty() ? "src" : manifest.sourceDir;
        fs::path sourcePath = fs::path(pkg.registryPath) / sourceDir;

        // Build the <Include> glob. If the declared source dir doesn't exist,
        // fall back to scanning the package root (some upstream packages have
        // a manifest source field that doesn't match the actual layout).
        std::string includeGlob;
        if (fs::exists(sourcePath, ec) && fs::is_directory(sourcePath, ec))
        {
            includeGlob = sourceDir + "/**/*.mt";
        }
        else
        {
            includeGlob = "**/*.mt";
            sourceDir = ".";
        }

        if (isUpToDate(pkg, sourceDir))
        {
            return true;  // No-op skip; caller counts this separately.
        }

        // Synthesize .mtproj
        std::string xml = buildSyntheticMtproj(pkg, includeGlob, resolved);
        fs::path mtprojPath = fs::path(pkg.registryPath) / "_mtype-autogen.mtproj";

        try
        {
            std::ofstream out(mtprojPath);
            if (!out)
            {
                error = "could not write " + mtprojPath.string();
                return false;
            }
            out << xml;
        }
        catch (const std::exception& e)
        {
            error = std::string("write synthetic .mtproj failed: ") + e.what();
            return false;
        }

        std::string subprocessOutput;
        int rc = runMTypeBuild(mtprojPath.string(), subprocessOutput);

        // Remove the synthetic file. Keep the .mtcLib (that's the artifact).
        std::error_code rmEc;
        fs::remove(mtprojPath, rmEc);

        if (rc != 0)
        {
            std::ostringstream msg;
            msg << "mType --build --lib failed (exit " << rc << ")";
            if (!subprocessOutput.empty())
            {
                msg << ":\n  " << subprocessOutput;
            }
            error = msg.str();
            return false;
        }

        fs::path produced = fs::path(pkg.registryPath) / (pkg.name + ".mtcLib");
        if (!fs::exists(produced, ec))
        {
            error = "mType --build --lib reported success but no .mtcLib was produced at " + produced.string();
            return false;
        }
        return true;
    }

    PackageCompiler::CompileResult PackageCompiler::compileAll(
        const std::unordered_map<std::string, ResolvedPackage>& resolved)
    {
        CompileResult result;

        if (resolved.empty())
        {
            return result;
        }

        if (mTypeExePath.empty())
        {
            // No explicit path and no fallback succeeded — caller likely didn't
            // call setMTypeExecutable. Surface a clear error rather than a
            // cryptic exec failure.
            result.success = false;
            result.errors.push_back(
                "mType.exe location unknown. Build with --lib bypasses compile step.");
            return result;
        }

        auto order = topologicalOrder(resolved);

        for (const auto& name : order)
        {
            auto it = resolved.find(name);
            if (it == resolved.end()) continue;
            const auto& pkg = it->second;

            fs::path libPath = fs::path(pkg.registryPath) / (pkg.name + ".mtcLib");
            std::error_code ec;
            auto timeBefore = fs::exists(libPath, ec)
                ? fs::last_write_time(libPath, ec)
                : std::filesystem::file_time_type{};

            std::string error;
            reportProgress("  Compiling " + pkg.name + "@" + pkg.version + " ...");
            if (!compileOne(pkg, resolved, error))
            {
                result.success = false;
                result.errors.push_back(pkg.name + "@" + pkg.version + ": " + error);
                continue;
            }

            // Differentiate skipped (file untouched) from compiled (file rewritten)
            // by comparing mtime. compileOne returns early on up-to-date without
            // writing, so the timestamp survives unchanged.
            std::error_code afterEc;
            auto timeAfter = fs::last_write_time(libPath, afterEc);
            if (!afterEc && timeAfter == timeBefore)
            {
                ++result.skipped;
            }
            else
            {
                ++result.compiled;
            }
        }

        return result;
    }
}
