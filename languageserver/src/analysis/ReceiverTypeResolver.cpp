#include "ReceiverTypeResolver.hpp"

#include "AstPositionIndex.hpp"

#include "../../../mType/ast/ASTNode.hpp"
#include "../../../mType/ast/nodes/expressions/VariableNode.hpp"
#include "../../../mType/ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../mType/ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../mType/ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodCallNode.hpp"
#include "../../../mType/ast/nodes/classes/NewNode.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/statements/AssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/ForEachNode.hpp"
#include "../../../mType/ast/GenericType.hpp"

#include "../../../mType/environment/Environment.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"
#include "../../../mType/environment/registry/FieldDefinition.hpp"
#include "../../../mType/environment/registry/FunctionDefinition.hpp"
#include "../../../mType/environment/registry/InterfaceDefinition.hpp"

#include <unordered_set>

namespace mtype::lsp {

types::UnifiedTypePtr unifiedTypeFromClassName(const std::string& className) {
    if (className.empty()) return nullptr;

    // Strip trailing `[]` pairs and wrap in arrayOf for each one. So "Foo[][]"
    // becomes arrayOf(arrayOf(classType("Foo"))). The mType parser stores
    // array decls as `className = "Foo[]"` on AssignmentNode (see
    // StatementParser::parseDeclaration), so this is the place to recover
    // array-ness for receiver chains like `arr[0].method()`.
    std::string base = className;
    int arrayDepth = 0;
    while (base.size() >= 2 && base.compare(base.size() - 2, 2, "[]") == 0) {
        base.resize(base.size() - 2);
        ++arrayDepth;
    }

    // Strip any trailing `<...>` generic-argument suffix — the LSP only keys
    // by base class name (mirrors DocumentManager's existing `lt = varType
    // .find('<')` trim in findDefinition / getTypeInfo).
    if (auto lt = base.find('<'); lt != std::string::npos) {
        base = base.substr(0, lt);
    }

    if (base.empty()) return nullptr;

    types::UnifiedTypePtr ut = types::UnifiedType::classType(base);
    for (int i = 0; i < arrayDepth; ++i) {
        ut = types::UnifiedType::arrayOf(ut);
    }
    return ut;
}

namespace {

// Strip outer array wrappers to get the "innermost" class name (matches the
// way DocumentManager keys symbolLocations).
std::string baseName(const types::UnifiedTypePtr& ut) {
    if (!ut) return {};
    types::UnifiedTypePtr cur = ut;
    while (cur && cur->isArray()) {
        cur = cur->getElementType();
    }
    if (!cur) return {};
    return cur->getName();
}

}  // namespace

ReceiverTypeResolver::ReceiverTypeResolver(
    std::shared_ptr<environment::Environment> env,
    const std::vector<std::unique_ptr<ast::ASTNode>>* documentAst,
    int cursorLine, int cursorCol)
    : env_(std::move(env)),
      documentAst_(documentAst),
      cursorLine_(cursorLine),
      cursorCol_(cursorCol) {}

types::UnifiedTypePtr ReceiverTypeResolver::resolve(const ast::ASTNode* receiver) const {
    if (!receiver) return nullptr;

    if (auto* n = dynamic_cast<const ast::nodes::expressions::VariableNode*>(receiver)) {
        return resolveVariable(*n);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::MemberAccessNode*>(receiver)) {
        return resolveMemberAccess(*n);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::MethodCallNode*>(receiver)) {
        return resolveMethodCall(*n);
    }
    if (auto* n = dynamic_cast<const ast::nodes::functions::FunctionCallNode*>(receiver)) {
        return resolveFunctionCall(*n);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::IndexAccessNode*>(receiver)) {
        return resolveIndexAccess(*n);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::TernaryExpNode*>(receiver)) {
        return resolveTernary(*n);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::NewNode*>(receiver)) {
        return resolveNew(*n);
    }
    return nullptr;
}

std::string ReceiverTypeResolver::resolveName(const ast::ASTNode* receiver) const {
    return baseName(resolve(receiver));
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveVariable(
    const ast::nodes::expressions::VariableNode& node) const {
    return lookupVariableType(node.getName());
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveMemberAccess(
    const ast::nodes::classes::MemberAccessNode& node) const {
    if (!env_) return nullptr;
    auto classReg = env_->getClassRegistry();
    if (!classReg) return nullptr;

    // For static access (`Class::field`), the LHS variable's name *is* the
    // class name. lookupVariableType handles this via the class-name
    // shortcut, so the same code path serves both static and instance.
    auto ownerType = resolve(node.getObject());
    std::string ownerClass = baseName(ownerType);
    if (ownerClass.empty()) return nullptr;

    auto cls = classReg->findClass(ownerClass);
    if (!cls) return nullptr;

    std::shared_ptr<runtimeTypes::klass::FieldDefinition> field;
    if (node.getIsStaticAccess()) {
        const auto& staticFields = cls->getStaticFields();
        auto it = staticFields.find(node.getMemberName());
        if (it != staticFields.end()) field = it->second;
    } else {
        field = cls->getFieldInHierarchy(node.getMemberName());
    }
    // MYT-358 parity — when static lookup misses, fall back to instance fields.
    if (!field) {
        const auto& instanceFields = cls->getInstanceFields();
        auto it = instanceFields.find(node.getMemberName());
        if (it != instanceFields.end()) field = it->second;
    }
    if (!field) return nullptr;
    return field->getUnifiedType();
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveMethodCall(
    const ast::nodes::classes::MethodCallNode& node) const {
    if (!env_) return nullptr;
    auto classReg = env_->getClassRegistry();
    if (!classReg) return nullptr;

    auto ownerType = resolve(node.getObject());
    std::string ownerClass = baseName(ownerType);
    if (ownerClass.empty()) return nullptr;

    const size_t argCount = node.getArguments().size();

    if (auto cls = classReg->findClass(ownerClass)) {
        auto overloads = node.getIsStaticCall()
            ? cls->getAllStaticMethodOverloads(node.getMethodName())
            : cls->getAllInstanceMethodOverloads(node.getMethodName());
        if (overloads.empty()) {
            // Mirror DocumentManager's instance-fallback for upper-case receivers
            // whose method is actually instance-side (or vice versa).
            overloads = node.getIsStaticCall()
                ? cls->getAllInstanceMethodOverloads(node.getMethodName())
                : cls->getAllStaticMethodOverloads(node.getMethodName());
        }
        if (!overloads.empty()) {
            // Coarse overload disambiguation: pick the first overload matching the
            // call's argument count, else the first overload. Generic-arg inference
            // is explicitly out of scope per the MYT-359 ticket.
            for (const auto& m : overloads) {
                if (m && m->getParameters().size() == argCount) {
                    return m->getUnifiedReturnType();
                }
            }
            if (overloads.front()) return overloads.front()->getUnifiedReturnType();
        }
        return nullptr;
    }

    // MYT-362 — receiver typed as an interface (e.g. `Stream<E>` returned by
    // `ArrayList.stream()`). The ClassRegistry misses it because interfaces
    // live in their own registry. Walk this interface and its parents (BFS,
    // visited-set cycle guard) and return the first matching method
    // signature's return type, preferring an arg-count match.
    return resolveInterfaceMethodReturn(ownerClass, node.getMethodName(), argCount);
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveInterfaceMethodReturn(
    const std::string& interfaceName,
    const std::string& methodName,
    size_t argCount) const {
    if (!env_ || interfaceName.empty()) return nullptr;

    std::unordered_set<std::string> visited;
    std::vector<std::string> stack;
    stack.push_back(interfaceName);

    const runtimeTypes::klass::MethodSignature* anyMatch = nullptr;

    while (!stack.empty()) {
        std::string current = std::move(stack.back());
        stack.pop_back();
        if (!visited.insert(current).second) continue;

        auto iface = env_->findInterface(current);
        if (!iface) continue;

        for (const auto& sig : iface->getMethodSignatures()) {
            if (sig.name != methodName) continue;
            if (sig.parameters.size() == argCount) {
                return sig.returnType;
            }
            if (!anyMatch) anyMatch = &sig;
        }
        for (const auto& parent : iface->getExtendedInterfaces()) {
            if (!visited.count(parent)) stack.push_back(parent);
        }
    }

    return anyMatch ? anyMatch->returnType : nullptr;
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveFunctionCall(
    const ast::nodes::functions::FunctionCallNode& node) const {
    if (!env_) return nullptr;
    auto func = env_->findFunction(node.getFunctionName());
    if (!func) return nullptr;
    return func->getUnifiedReturnType();
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveIndexAccess(
    const ast::nodes::expressions::IndexAccessNode& node) const {
    auto collType = resolve(node.getCollection());
    if (!collType) return nullptr;
    if (collType->isArray()) {
        return collType->getElementType();
    }
    // Parameterized collection types (List<Foo>, ArrayList<Foo>) — first type
    // argument is the element. We don't try to validate that the receiver
    // actually implements an indexable interface; LSP best-effort.
    const auto& args = collType->getTypeArguments();
    if (!args.empty()) {
        return args.front();
    }
    return nullptr;
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveTernary(
    const ast::nodes::expressions::TernaryExpNode& node) const {
    if (auto t = resolve(node.getTrueExpression())) return t;
    return resolve(node.getFalseExpression());
}

types::UnifiedTypePtr ReceiverTypeResolver::resolveNew(
    const ast::nodes::classes::NewNode& node) const {
    return unifiedTypeFromClassName(node.getClassName());
}

types::UnifiedTypePtr ReceiverTypeResolver::lookupVariableType(
    const std::string& name) const {
    if (name.empty()) return nullptr;

    if ((name == "this" || name == "super") && documentAst_) {
        if (auto* cls = findEnclosingClass(*documentAst_, cursorLine_, cursorCol_)) {
            if (name == "this") {
                return types::UnifiedType::classType(cls->getClassName());
            }
            if (cls->hasParentClass()) {
                return types::UnifiedType::classType(cls->getParentClassName());
            }
        }
    }

    // Class-name shortcut: `Audio` in `Audio::sndChord.play()` parses as a
    // VariableNode whose name happens to be a registered class — return the
    // class type so static-access chains type-check identically to instance.
    if (env_) {
        if (auto classReg = env_->getClassRegistry()) {
            if (auto cls = classReg->findClass(name)) {
                return types::UnifiedType::classType(cls->getName());
            }
        }
    }

    // MYT-359 — qualified `Class::staticField` form. `parseScopeResolution`
    // (ExpressionParser.cpp:637) collapses `Audio::sndChord` to a single
    // `VariableNode("Audio::sndChord")` whenever it isn't followed by `(`,
    // so chains like `Audio::sndChord.play()` and `App::outer.inner.play()`
    // both surface here with a `::` in the name. Split at the last `::`,
    // resolve the LHS as a class, and look up the RHS as a static field
    // (falling back to instance fields for parity with MYT-358).
    if (auto sep = name.rfind("::"); sep != std::string::npos && env_) {
        const std::string ownerName = name.substr(0, sep);
        const std::string memberName = name.substr(sep + 2);
        if (auto classReg = env_->getClassRegistry()) {
            if (auto cls = classReg->findClass(ownerName)) {
                std::shared_ptr<runtimeTypes::klass::FieldDefinition> field;
                const auto& staticFields = cls->getStaticFields();
                if (auto it = staticFields.find(memberName); it != staticFields.end()) {
                    field = it->second;
                }
                if (!field) {
                    const auto& instanceFields = cls->getInstanceFields();
                    if (auto it = instanceFields.find(memberName); it != instanceFields.end()) {
                        field = it->second;
                    }
                }
                if (field) {
                    if (auto ut = field->getUnifiedType()) return ut;
                }
            }
        }
    }

    // Local decl scan (most common path for `Foo f = ...` / `for (Foo f : ...)`).
    if (auto t = findLocalDeclType(name)) return t;

    // Parameter of the enclosing function/method/constructor.
    if (auto t = findParamType(name)) return t;

    // Top-level / global variable registered in the environment (rare).
    if (env_) {
        if (auto v = env_->findVariable(name)) {
            if (auto ut = v->getUnifiedType()) return ut;
            if (!v->getClassName().empty()) {
                return unifiedTypeFromClassName(v->getClassName());
            }
        }
    }
    return nullptr;
}

types::UnifiedTypePtr ReceiverTypeResolver::findLocalDeclType(
    const std::string& name) const {
    if (!documentAst_) return nullptr;

    types::UnifiedTypePtr best;
    int bestLine = -1;

    walkAst(*documentAst_, [&](const ast::ASTNode* n) -> bool {
        if (auto* a = dynamic_cast<const ast::nodes::statements::AssignmentNode*>(n)) {
            if (a->getVariableName() != name) return false;
            if (a->getVariableType() == value::ValueType::VOID) return false;  // re-assignment, not a decl
            int line = a->getLocation().getLine();
            if (line > cursorLine_ + 1) return false;  // decl must precede the cursor
            if (line <= bestLine) return false;
            std::string cls = a->getClassName();
            types::UnifiedTypePtr ut = cls.empty()
                ? types::UnifiedType::primitive(a->getVariableType())
                : unifiedTypeFromClassName(cls);
            if (ut) {
                best = ut;
                bestLine = line;
            }
        } else if (auto* f = dynamic_cast<const ast::nodes::statements::ForEachNode*>(n)) {
            if (f->getVariableName() != name) return false;
            int line = f->getLocation().getLine();
            if (line > cursorLine_ + 1) return false;
            if (line <= bestLine) return false;
            const auto& tinfo = f->getVariableTypeInfo();
            types::UnifiedTypePtr ut = tinfo.className.empty()
                ? types::UnifiedType::primitive(tinfo.baseType)
                : unifiedTypeFromClassName(tinfo.className);
            if (ut) {
                best = ut;
                bestLine = line;
            }
        }
        return false;
    });

    return best;
}

types::UnifiedTypePtr ReceiverTypeResolver::findParamType(
    const std::string& name) const {
    if (!documentAst_) return nullptr;

    auto* enclosing = findEnclosingCallable(*documentAst_, cursorLine_, cursorCol_);
    if (!enclosing) return nullptr;

    auto fromGenericList = [&](const auto& params) -> types::UnifiedTypePtr {
        for (const auto& [pname, gt] : params) {
            if (pname != name) continue;
            if (!gt) return nullptr;
            std::string base = gt->getBaseTypeName();
            if (base.empty()) return nullptr;
            return types::UnifiedType::classType(base);
        }
        return nullptr;
    };

    if (auto* f = dynamic_cast<const ast::nodes::functions::FunctionNode*>(enclosing)) {
        return fromGenericList(f->getGenericParameters());
    }
    if (auto* m = dynamic_cast<const ast::nodes::classes::MethodNode*>(enclosing)) {
        return fromGenericList(m->getGenericParameters());
    }
    // ConstructorNode params live in a different shape; skip — receiver
    // chains keyed on a constructor parameter are vanishingly rare in mType
    // code, and the LSP gracefully falls back to nullptr here.
    return nullptr;
}

}  // namespace mtype::lsp
