#pragma once

#include "../DocumentManager.hpp"
#include "../analysis/WorkspaceSymbolIndex.hpp"
#include "../utils/LSPTypes.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mtype::lsp {

// Call hierarchy support: prepare → incomingCalls / outgoingCalls.
//
// v1 scope (see plan): top-level functions, class methods (instance and
// static), constructors with an explicit ConstructorNode. Resolution is
// hybrid — class-pinned where syntax pins it (Class::staticMethod,
// new Class(), super.method(), this.method() inside class C), name-only
// otherwise. Lambdas are transparent: calls inside their bodies bubble up
// to the enclosing real callable. Workspace scope mirrors ReferencesHandler
// — only documents loaded in DocumentManager are scanned. Overloads of the
// same (name, class) collapse into one CallHierarchyItem; per-overload
// precision would require argument-type inference (out of scope).
class CallHierarchyHandler {
public:
    CallHierarchyHandler(DocumentManager* docMgr,
                         std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex = nullptr);

    // textDocument/prepareCallHierarchy
    std::vector<CallHierarchyItem> handlePrepare(const std::string& uri,
                                                 const Position& position);

    // callHierarchy/incomingCalls
    std::vector<CallHierarchyIncomingCall> handleIncoming(const CallHierarchyItem& item);

    // callHierarchy/outgoingCalls
    std::vector<CallHierarchyOutgoingCall> handleOutgoing(const CallHierarchyItem& item);

private:
    DocumentManager* documentManager_;
    std::shared_ptr<analysis::WorkspaceSymbolIndex> workspaceIndex_;
};

} // namespace mtype::lsp
