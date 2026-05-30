#include "LifecycleCodegen.hpp"

#include "AccessorCodegen.hpp"

#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"

#include <array>
#include <sstream>
#include <utility>

namespace mtype::lsp::lifecyclegen {

using ast::nodes::classes::ClassNode;
using ast::nodes::classes::MethodNode;

bool hasMethod(const ClassNode& cls, const std::string& name) {
    for (const auto& node : cls.getMethods()) {
        const auto* method = dynamic_cast<const MethodNode*>(node.get());
        if (method && method->getName() == name) {
            return true;
        }
    }
    return false;
}

std::string buildLifecycleBody(const ClassNode& cls) {
    // The canonical @Script lifecycle contract: method name -> parameter list.
    // Signatures match what AnnotationValidator_Constraints reports.
    static const std::array<std::pair<const char*, const char*>, 3> lifecycle{{
        {"onStart", ""},
        {"onUpdate", "float deltaTime"},
        {"onDestroy", ""},
    }};

    std::ostringstream body;
    bool generatedAny = false;

    // Constructor first -- a concrete @Script class also needs a no-arg
    // constructor. Reuse the accessor module so the constructor text and its
    // duplicate detection stay identical to the standalone constructor action.
    const std::string constructor = accessorgen::buildDefaultConstructorBody(cls);
    if (!constructor.empty()) {
        body << constructor << "\n";
        generatedAny = true;
    }

    for (const auto& [name, params] : lifecycle) {
        if (hasMethod(cls, name)) continue;
        body << "    public function " << name << "(" << params << "): void {\n"
             << "    }\n\n";
        generatedAny = true;
    }

    return generatedAny ? body.str() : std::string();
}

} // namespace mtype::lsp::lifecyclegen
