#include "ImportUtils.hpp"

namespace mtype::lsp::utils
{
    TextEdit buildImportInsertEdit(
        int insertLine,
        const std::string& identifier,
        const std::string& spelling)
    {
        TextEdit edit;
        edit.range.start.line = insertLine;
        edit.range.start.character = 0;
        edit.range.end = edit.range.start;
        edit.newText = "import " + identifier + " from \"" + spelling + "\";\n";
        return edit;
    }
}
