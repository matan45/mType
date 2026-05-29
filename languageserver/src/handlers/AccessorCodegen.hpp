#pragma once

#include <string>
#include <vector>

namespace ast::nodes::classes
{
    class ClassNode;
    class FieldNode;
}

// Pure text generation for the "Generate getters and setters" and
// "Generate default constructor" code actions. Kept free of LSP types
// and DocumentManager so the codegen is unit-testable in isolation and
// so CodeActionHandler.cpp stays thin. Output matches mType's stdlib
// style (4-space indent, `this.field` access) and the naming the
// compile-time Lombok pass uses (get/set + capitalized field name), so
// generated source is byte-compatible with what `@Getter`/`@Setter`
// would synthesize.
namespace mtype::lsp::accessorgen {

// "name" -> "Name"; leaves an empty string unchanged.
std::string capitalize(const std::string& s);

// Renders a field's declared type via GenericType::toString()
// ("int", "String", "List<String>", "Person", "int[]"); falls back to
// "object" when the field has no generic type attached.
std::string fieldTypeToString(const ast::nodes::classes::FieldNode& field);

// Own instance fields only — static fields are skipped (accessors and
// no-arg constructors don't apply to them).
std::vector<const ast::nodes::classes::FieldNode*>
eligibleFields(const ast::nodes::classes::ClassNode& cls);

// Duplicate detection — scan existing members so we never regenerate
// one the user already wrote.
bool hasGetter(const ast::nodes::classes::ClassNode& cls, const std::string& field);
bool hasSetter(const ast::nodes::classes::ClassNode& cls, const std::string& field);
bool hasNoArgConstructor(const ast::nodes::classes::ClassNode& cls);

// Per-field method text, each ending with the closing-brace line.
std::string buildGetterText(const ast::nodes::classes::FieldNode& field);
std::string buildSetterText(const ast::nodes::classes::FieldNode& field);

// Aggregate getter/setter block for every eligible field that lacks an
// accessor (final fields get a getter but no setter). Returns "" when
// nothing needs generating so the caller can skip offering the action.
std::string buildAccessorsBody(const ast::nodes::classes::ClassNode& cls);

// "    public constructor() {\n    }\n"; returns "" when the class
// already declares a zero-arg constructor.
std::string buildDefaultConstructorBody(const ast::nodes::classes::ClassNode& cls);

} // namespace mtype::lsp::accessorgen
