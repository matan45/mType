#include "src/MTypeLanguageServer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        mtype::lsp::MTypeLanguageServer server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
