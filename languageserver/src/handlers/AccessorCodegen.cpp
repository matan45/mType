#include "AccessorCodegen.hpp"

#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"
#include "../../../mType/optimizer/passes/shared/AccessorNaming.hpp"

#include <sstream>

namespace mtype::lsp::accessorgen {

namespace shared = optimizer::passes::shared;

using ast::nodes::classes::ClassNode;
using ast::nodes::classes::ConstructorNode;
using ast::nodes::classes::FieldNode;
using ast::nodes::classes::MethodNode;

// Delegates to the single shared definition so the LSP code action and the
// compiler-side @Getter/@Setter synthesis derive identical method names.
std::string capitalize(const std::string& s) {
    return shared::capitalize(s);
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
        fields.push_back(field);
    }
    return fields;
}

bool hasGetter(const ClassNode& cls, const std::string& field) {
    const std::string target = shared::getterName(field);
    for (const auto& node : cls.getMethods()) {
        const auto* method = dynamic_cast<const MethodNode*>(node.get());
        if (method && method->getName() == target && method->getParameterCount() == 0) {
            return true;
        }
    }
    return false;
}

bool hasSetter(const ClassNode& cls, const std::string& field) {
    const std::string target = shared::setterName(field);
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

std::string buildGetterText(const FieldNode& field, const std::string& className) {
    const std::string& name = field.getName();
    const bool isStatic = field.getIsStatic();
    // Static fields are referenced as `ClassName::field`; instance
    // fields via `this.field`.
    const std::string access = isStatic ? className + "::" + name : "this." + name;
    std::ostringstream out;
    out << "    public " << (isStatic ? "static " : "") << "function " << shared::getterName(name)
        << "(): " << fieldTypeToString(field) << " {\n";
    out << "        return " << access << ";\n";
    out << "    }\n";
    return out.str();
}

std::string buildSetterText(const FieldNode& field, const std::string& className) {
    const std::string& name = field.getName();
    const bool isStatic = field.getIsStatic();
    const std::string access = isStatic ? className + "::" + name : "this." + name;
    std::ostringstream out;
    out << "    public " << (isStatic ? "static " : "") << "function " << shared::setterName(name)
        << "(" << fieldTypeToString(field) << " " << name << "): void {\n";
    out << "        " << access << " = " << name << ";\n";
    out << "    }\n";
    return out.str();
}

std::string buildAccessorsBody(const ClassNode& cls) {
    const std::string& className = cls.getClassName();
    std::ostringstream body;
    bool generatedAny = false;
    for (const auto* field : eligibleFields(cls)) {
        const std::string& name = field->getName();
        if (!hasGetter(cls, name)) {
            body << buildGetterText(*field, className) << "\n";
            generatedAny = true;
        }
        // A setter assigning a `final` field is invalid mType, so final
        // fields get a getter only.
        if (!field->getIsFinal() && !hasSetter(cls, name)) {
            body << buildSetterText(*field, className) << "\n";
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
