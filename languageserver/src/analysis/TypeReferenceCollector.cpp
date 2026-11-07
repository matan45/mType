#include "TypeReferenceCollector.hpp"

#include "AstPositionIndex.hpp"

#include "../../../mType/ast/ASTNode.hpp"
#include "../../../mType/ast/GenericType.hpp"
#include "../../../mType/ast/GenericTypeParameter.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodCallNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/NewNode.hpp"
#include "../../../mType/ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../mType/ast/nodes/expressions/LambdaNode.hpp"
#include "../../../mType/ast/nodes/expressions/VariableNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/statements/AssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/CatchNode.hpp"
#include "../../../mType/ast/nodes/statements/ForEachNode.hpp"
#include "../../../mType/value/ParameterType.hpp"

#include <cctype>

namespace mtype::lsp {

namespace {

namespace classes = ast::nodes::classes;
namespace expressions = ast::nodes::expressions;
namespace functions = ast::nodes::functions;
namespace statements = ast::nodes::statements;

using GenericParams = std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>;

void addTypeExpr(TypeReferences& out, const std::string& typeExpr) {
    for (auto& name : TypeReferenceCollector::splitTypeNames(typeExpr)) {
        out.referenced.insert(std::move(name));
    }
}

void addGenericType(TypeReferences& out, const std::shared_ptr<ast::GenericType>& type) {
    if (type) addTypeExpr(out, type->toString());
}

void addGenericParams(TypeReferences& out, const std::vector<ast::GenericTypeParameter>& params) {
    for (const auto& param : params) out.genericParams.insert(param.name);
}

// `Foo::bar()` and `Foo::X` name the class `Foo`.
void addStaticHead(TypeReferences& out, const std::string& qualified) {
    const auto scope = qualified.find("::");
    if (scope != std::string::npos && scope > 0) {
        out.referenced.insert(qualified.substr(0, scope));
    }
}

void addSignature(TypeReferences& out, const GenericParams& params,
                  const std::shared_ptr<ast::GenericType>& returnType) {
    for (const auto& [name, type] : params) {
        out.valueNames.insert(name);
        addGenericType(out, type);
    }
    addGenericType(out, returnType);
}

void addParameterType(TypeReferences& out, const value::ParameterType& type) {
    if (type.className) addTypeExpr(out, *type.className);
    if (type.interfaceName) addTypeExpr(out, *type.interfaceName);
    if (type.arrayElementTypeName) addTypeExpr(out, *type.arrayElementTypeName);
}

bool visitTypeDeclaration(const ast::ASTNode* node, TypeReferences& out) {
    if (const auto* cls = dynamic_cast<const classes::ClassNode*>(node)) {
        out.declaredTypes.insert(cls->getClassName());
        addGenericParams(out, cls->getGenericParameters());
        if (cls->hasParentClass()) addTypeExpr(out, cls->getParentClassName());
        for (const auto& iface : cls->getImplementedInterfaces()) addTypeExpr(out, iface);
        return true;
    }
    if (const auto* iface = dynamic_cast<const classes::InterfaceNode*>(node)) {
        out.declaredTypes.insert(iface->getName());
        addGenericParams(out, iface->getGenericParameters());
        for (const auto& parent : iface->getExtendedInterfaces()) addTypeExpr(out, parent);
        return true;
    }
    return false;
}

bool visitMemberDeclaration(const ast::ASTNode* node, TypeReferences& out) {
    if (const auto* field = dynamic_cast<const classes::FieldNode*>(node)) {
        out.valueNames.insert(field->getName());
        addGenericType(out, field->getGenericType());
        return true;
    }
    if (const auto* fn = dynamic_cast<const functions::FunctionNode*>(node)) {
        out.valueNames.insert(fn->getName());
        addGenericParams(out, fn->getGenericTypeParameters());
        addSignature(out, fn->getGenericParameters(), fn->getGenericReturnType());
        return true;
    }
    if (const auto* method = dynamic_cast<const classes::MethodNode*>(node)) {
        out.valueNames.insert(method->getName());
        addGenericParams(out, method->getGenericTypeParameters());
        addSignature(out, method->getGenericParameters(), method->getGenericReturnType());
        return true;
    }
    if (const auto* ctor = dynamic_cast<const classes::ConstructorNode*>(node)) {
        for (const auto& [name, type] : ctor->getParametersWithTypes()) {
            out.valueNames.insert(name);
            addParameterType(out, type);
        }
        return true;
    }
    return false;
}

bool visitStatement(const ast::ASTNode* node, TypeReferences& out) {
    if (const auto* assign = dynamic_cast<const statements::AssignmentNode*>(node)) {
        out.valueNames.insert(assign->getVariableName());
        addTypeExpr(out, assign->getClassName());
        return true;
    }
    if (const auto* katch = dynamic_cast<const statements::CatchNode*>(node)) {
        out.valueNames.insert(katch->getVariableName());
        addTypeExpr(out, katch->getExceptionType());
        return true;
    }
    if (const auto* each = dynamic_cast<const statements::ForEachNode*>(node)) {
        out.valueNames.insert(each->getVariableName());
        addTypeExpr(out, each->getVariableTypeInfo().className);
        return true;
    }
    return false;
}

bool visitExpression(const ast::ASTNode* node, TypeReferences& out) {
    if (const auto* created = dynamic_cast<const classes::NewNode*>(node)) {
        addTypeExpr(out, created->getClassName());
        return true;
    }
    if (const auto* array = dynamic_cast<const expressions::ArrayCreationNode*>(node)) {
        addTypeExpr(out, array->getElementTypeInfo().className);
        return true;
    }
    if (const auto* lambda = dynamic_cast<const expressions::LambdaNode*>(node)) {
        for (const auto& param : lambda->getParameters()) {
            out.valueNames.insert(param.name);
            addGenericType(out, param.type);
        }
        return true;
    }
    if (const auto* call = dynamic_cast<const functions::FunctionCallNode*>(node)) {
        addStaticHead(out, call->getFunctionName());
        for (const auto& arg : call->getGenericTypeArguments()) addTypeExpr(out, arg);
        return true;
    }
    if (const auto* call = dynamic_cast<const classes::MethodCallNode*>(node)) {
        // `Foo::m<T>()` parses as a static call on a VariableNode receiver.
        const auto* receiver = dynamic_cast<const expressions::VariableNode*>(call->getObject());
        if (call->getIsStaticCall() && receiver) out.referenced.insert(receiver->getName());
        for (const auto& arg : call->getGenericTypeArguments()) addTypeExpr(out, arg);
        return true;
    }
    if (const auto* variable = dynamic_cast<const expressions::VariableNode*>(node)) {
        addStaticHead(out, variable->getName());
        return true;
    }
    return false;
}

} // namespace

TypeReferences TypeReferenceCollector::collect(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots) {
    TypeReferences out;
    walkAst(roots, [&out](const ast::ASTNode* node) {
        if (!visitTypeDeclaration(node, out) && !visitMemberDeclaration(node, out)
            && !visitStatement(node, out)) {
            visitExpression(node, out);
        }
        return false; // never stop early
    });
    return out;
}

std::vector<std::string> TypeReferenceCollector::splitTypeNames(const std::string& typeExpr) {
    std::vector<std::string> names;
    std::string current;
    auto flush = [&]() {
        const bool qualified = current.find_first_of(":.") != std::string::npos;
        if (!current.empty() && !qualified
            && !std::isdigit(static_cast<unsigned char>(current.front()))) {
            names.push_back(current);
        }
        current.clear();
    };
    for (char c : typeExpr) {
        const auto uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == '_' || c == ':' || c == '.') {
            current.push_back(c);
        } else {
            flush();
        }
    }
    flush();
    return names;
}

} // namespace mtype::lsp
