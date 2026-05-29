#include "AccessorCodegen.hpp"

#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"

#include <cctype>
#include <sstream>

namespace mtype::lsp::accessorgen {

using ast::nodes::classes::ClassNode;
using ast::nodes::classes::ConstructorNode;
using ast::nodes::classes::FieldNode;
using ast::nodes::classes::MethodNode;

std::string capitalize(const std::string& s) {
    if (s.empty()) return s;
    std::string result = s;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

std::string fieldTypeToString(const FieldNode& field) {
    const auto type = field.getGenericType();
    return type ? type->toString() : std::string("object");
}

std::vector<const FieldNode*> eligibleFields(const ClassNode& cls) {
    std::vector<const FieldNode*> fields;
    for (const auto& node : cls.getFields()) {
        const auto* field = dynamic_cast<const FieldNode*>(node.get());
        if (!field) continue;
        if (field->getIsStatic()) continue;  // statics get no accessor/ctor param
        fields.push_back(field);
    }
    return fields;
}

bool hasGetter(const ClassNode& cls, const std::string& field) {
    const std::string target = "get" + capitalize(field);
    for (const auto& node : cls.getMethods()) {
        const auto* method = dynamic_cast<const MethodNode*>(node.get());
        if (method && method->getName() == target && method->getParameterCount() == 0) {
            return true;
        }
    }
    return false;
}

bool hasSetter(const ClassNode& cls, const std::string& field) {
    const std::string target = "set" + capitalize(field);
    for (const auto& node : cls.getMethods()) {
        const auto* method = dynamic_cast<const MethodNode*>(node.get());
        if (method && method->getName() == target && method->getParameterCount() == 1) {
            return true;
        }
    }
    return false;
}

bool hasNoArgConstructor(const ClassNode& cls) {
    for (const auto& node : cls.getConstructors()) {
        const auto* ctor = dynamic_cast<const ConstructorNode*>(node.get());
        if (ctor && ctor->getParameterCount() == 0) {
            return true;
        }
    }
    return false;
}

std::string buildGetterText(const FieldNode& field) {
    const std::string& name = field.getName();
    std::ostringstream out;
    out << "    public function get" << capitalize(name)
        << "(): " << fieldTypeToString(field) << " {\n";
    out << "        return this." << name << ";\n";
    out << "    }\n";
    return out.str();
}

std::string buildSetterText(const FieldNode& field) {
    const std::string& name = field.getName();
    std::ostringstream out;
    out << "    public function set" << capitalize(name)
        << "(" << fieldTypeToString(field) << " " << name << "): void {\n";
    out << "        this." << name << " = " << name << ";\n";
    out << "    }\n";
    return out.str();
}

std::string buildAccessorsBody(const ClassNode& cls) {
    std::ostringstream body;
    bool generatedAny = false;
    for (const auto* field : eligibleFields(cls)) {
        const std::string& name = field->getName();
        if (!hasGetter(cls, name)) {
            body << buildGetterText(*field) << "\n";
            generatedAny = true;
        }
        // A setter assigning a `final` field is invalid mType, so final
        // fields get a getter only.
        if (!field->getIsFinal() && !hasSetter(cls, name)) {
            body << buildSetterText(*field) << "\n";
            generatedAny = true;
        }
    }
    return generatedAny ? body.str() : std::string();
}

std::string buildDefaultConstructorBody(const ClassNode& cls) {
    if (hasNoArgConstructor(cls)) return std::string();
    return "    public constructor() {\n    }\n";
}

} // namespace mtype::lsp::accessorgen
