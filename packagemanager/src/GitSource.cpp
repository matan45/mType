#include "GitSource.hpp"
#include <filesystem>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <array>
#include <cstdio>

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

    static bool startsWith(const std::string& str, const std::string& prefix)
    {
        return str.size() >= prefix.size() &&
               str.compare(0, prefix.size(), prefix) == 0;
    }

    void GitSource::validateSafeString(const std::string& str, const std::string& context)
    {
        // Reject characters that could enable injection. The Windows runGit
        // path wraps each arg in "..." before handing the line to
        // CreateProcessA, so an unescaped " breaks out of its token and a
        // trailing \ escapes the closing quote — both end up reinterpreted
        // by CommandLineToArgvW in the child. Other shell metacharacters
        // are kept too as defence-in-depth for any future shell-using path.
        static const std::string forbidden = "&|;$`(){}!#\n\r\"\\";
        for (char c : str)
        {
            if (forbidden.find(c) != std::string::npos)
            {
                throw std::runtime_error(
                    "Invalid character '" + std::string(1, c) + "' in " + context +
                    ": " + str);
            }
        }
    }

    std::string GitSource::toCloneUrl(const std::string& source)
    {
        std::string url;

        if (startsWith(source, "github:"))
        {
            std::string path = source.substr(7);
            // Validate github path is user/repo format
            if (path.find('/') == std::string::npos)
            {
                throw std::runtime_error("Invalid github source, expected github:user/repo, got: " + source);
            }
            url = "https://github.com/" + path + ".git";
        }
        else if (startsWith(source, "https://") || startsWith(source, "http://") ||
                 startsWith(source, "git://"))
        {
            url = source;
        }
        else
        {
            throw std::runtime_error("Unknown git source format: " + source);
        }

        validateSafeString(url, "git URL");
        return url;
    }

    bool GitSource::isGitSource(const std::string& source)
    {
        return startsWith(source, "github:") ||
               startsWith(source, "https://") ||
               startsWith(source, "http://") ||
               startsWith(source, "git://");
    }

#ifdef _WIN32
    std::string GitSource::runGit(const std::vector<std::string>& args)
    {
        // Build command line for CreateProcess (no shell)
        std::string cmdLine = "git";
        for (const auto& arg : args)
        {
            cmdLine += " \"" + arg + "\"";
        }

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE readPipe, writePipe;
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        {
            throw std::runtime_error("Failed to create pipe for git");
        }
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError = writePipe;

        PROCESS_INFORMATION pi{};

        BOOL ok = CreateProcessA(
            nullptr,
            const_cast<char*>(cmdLine.c_str()),
            nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW,
            nullptr, nullptr,
            &si, &pi);

        CloseHandle(writePipe);

        if (!ok)
        {
            CloseHandle(readPipe);
            throw std::runtime_error("Failed to run git command");
        }

        std::string output;
        char buffer[256];
        DWORD bytesRead;
        while (ReadFile(readPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        CloseHandle(readPipe);

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0)
        {
            throw std::runtime_error("git command failed (exit " + std::to_string(exitCode) + "): " + cmdLine);
        }

        return output;
    }

    int GitSource::runGitStatus(const std::vector<std::string>& args)
    {
        std::string cmdLine = "git";
        for (const auto& arg : args)
        {
            cmdLine += " \"" + arg + "\"";
        }

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi{};

        BOOL ok = CreateProcessA(
            nullptr,
            const_cast<char*>(cmdLine.c_str()),
            nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW,
            nullptr, nullptr,
            &si, &pi);

        if (!ok)
        {
            return -1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return static_cast<int>(exitCode);
    }

#else
    // POSIX: fork + execvp (no shell)
    std::string GitSource::runGit(const std::vector<std::string>& args)
    {
        int pipefd[2];
        if (pipe(pipefd) != 0)
        {
            throw std::runtime_error("Failed to create pipe for git");
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            close(pipefd[0]);
            close(pipefd[1]);
            throw std::runtime_error("Failed to fork for git");
        }

        if (pid == 0)
        {
            // Child
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            std::vector<const char*> argv;
            argv.push_back("git");
            for (const auto& a : args) argv.push_back(a.c_str());
            argv.push_back(nullptr);

            execvp("git", const_cast<char* const*>(argv.data()));
            _exit(127);
        }

        // Parent
        close(pipefd[1]);

        std::string output;
        char buffer[256];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[n] = '\0';
            output += buffer;
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            throw std::runtime_error("git command failed (exit " + std::to_string(code) + ")");
        }

        return output;
    }

    int GitSource::runGitStatus(const std::vector<std::string>& args)
    {
        pid_t pid = fork();
        if (pid < 0) return -1;

        if (pid == 0)
        {
            // Suppress output
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0)
            {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }

            std::vector<const char*> argv;
            argv.push_back("git");
            for (const auto& a : args) argv.push_back(a.c_str());
            argv.push_back(nullptr);

            execvp("git", const_cast<char* const*>(argv.data()));
            _exit(127);
        }

        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
#endif

    static std::string extractVersionFromTagRef(const std::string& line)
    {
        const std::string prefix = "refs/tags/";
        size_t pos = line.find(prefix);
        if (pos == std::string::npos) return "";

        std::string tag = line.substr(pos + prefix.size());

        while (!tag.empty() && (tag.back() == '\n' || tag.back() == '\r' || tag.back() == ' '))
        {
            tag.pop_back();
        }

        if (tag.find("^{") != std::string::npos) return "";

        std::string version = tag;
        if (!version.empty() && version[0] == 'v')
        {
            version = version.substr(1);
        }

        int dots = 0;
        for (char c : version)
        {
            if (c == '.') ++dots;
            else if (c < '0' || c > '9') return "";
        }
        if (dots != 2) return "";

        return version;
    }

    std::vector<SemVer> GitSource::listRemoteTags(const std::string& cloneUrl)
    {
        validateSafeString(cloneUrl, "git URL");

        std::string output = runGit({"ls-remote", "--tags", cloneUrl});

        std::vector<SemVer> versions;
        std::istringstream stream(output);
        std::string line;

        while (std::getline(stream, line))
        {
            std::string version = extractVersionFromTagRef(line);
            if (!version.empty())
            {
                try
                {
                    versions.push_back(SemVer::parse(version));
                }
                catch (const std::exception&) {}
            }
        }

        std::sort(versions.begin(), versions.end());
        return versions;
    }

    std::string GitSource::fetchIntoRegistry(const std::string& cloneUrl,
                                              const std::string& packageName,
                                              const std::string& version,
                                              const std::string& registryRoot)
    {
        validateSafeString(cloneUrl, "git URL");
        validateSafeString(packageName, "package name");
        validateSafeString(version, "version");

        fs::path destPath = fs::path(registryRoot) / packageName / version;

        // Defence against path traversal: validateSafeString lets `.` and `/`
        // through (version strings need dots), so a packageName like
        // "../../etc/passwd" would resolve outside registryRoot. Canonicalise
        // both ends and require destPath stays under registryRoot before any
        // filesystem mutation.
        {
            fs::path normalisedRoot = fs::weakly_canonical(fs::path(registryRoot));
            fs::path normalisedDest = fs::weakly_canonical(destPath);
            const std::string rootStr = normalisedRoot.string();
            const std::string destStr = normalisedDest.string();
            if (!startsWith(destStr, rootStr))
            {
                throw std::runtime_error(
                    "Refusing package install: resolved path '" + destStr +
                    "' escapes registry root '" + rootStr + "'");
            }
        }

        if (fs::exists(destPath / "mtpkg.json"))
        {
            return destPath.string();
        }

        fs::create_directories(destPath);

        fs::path tempDir = fs::temp_directory_path() / ("_mtype_git_" + packageName + "_" + version);
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }

        // Try tag with 'v' prefix first, then without
        std::string tag = "v" + version;
        int result = runGitStatus({"clone", "--depth", "1", "--branch", tag, cloneUrl, tempDir.string()});
        if (result != 0)
        {
            tag = version;
            result = runGitStatus({"clone", "--depth", "1", "--branch", tag, cloneUrl, tempDir.string()});
        }

        if (result != 0)
        {
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            if (fs::exists(destPath)) fs::remove_all(destPath);
            throw std::runtime_error(
                "Failed to clone " + cloneUrl + " at tag " + version +
                ". Ensure the repo exists and has a matching tag (v" + version + " or " + version + ").");
        }

        fs::path gitDir = tempDir / ".git";
        if (fs::exists(gitDir))
        {
            fs::remove_all(gitDir);
        }

        fs::copy(tempDir, destPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        fs::remove_all(tempDir);

        if (!fs::exists(destPath / "mtpkg.json"))
        {
            throw std::runtime_error(
                "Cloned " + cloneUrl + "@" + version +
                " but no mtpkg.json found. Is this an mType package?");
        }

        return destPath.string();
    }

    void GitSource::createTag(const std::string& version)
    {
        validateSafeString(version, "version");
        int rc = runGitStatus({"tag", "v" + version});
        if (rc != 0)
        {
            throw std::runtime_error(
                "git tag v" + version + " failed (exit " + std::to_string(rc) +
                "). Ensure the current directory is a git repository and the tag does not already exist.");
        }
    }
}
