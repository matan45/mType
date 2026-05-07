#include "src/MTypeLanguageServer.hpp"
#include "version/Version.hpp"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string arg = argv[1];
        if (arg == "--version" || arg == "-v") {
            std::cout << "mtype-language-server " << mType::version::getVersionString() << "\n";
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << "mtype-language-server " << mType::version::getVersionString() << "\n\n";
            std::cout << "Usage:\n";
            std::cout << "  " << argv[0] << "              - Run language server over stdio\n";
            std::cout << "  " << argv[0] << " --version   - Print version\n";
            std::cout << "  " << argv[0] << " --help      - Show this help\n";
            return 0;
        }
    }

    // Set stdin/stdout to binary mode on Windows to avoid CR/LF issues
    #ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    #endif

    try {
        mtype::lsp::MTypeLanguageServer server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
