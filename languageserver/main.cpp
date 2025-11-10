#include "src/MTypeLanguageServer.hpp"
#include <iostream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

int main(int argc, char* argv[]) {
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
