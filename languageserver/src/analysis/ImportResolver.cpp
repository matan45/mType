#include "ImportResolver.hpp"
#include "SymbolRegistrationVisitor.hpp"
#include "../utils/MemoryFileReader.hpp"
#include "../utils/UriUtils.hpp"
#include "../../../mType/lexer/Lexer.hpp"
#include "../../../mType/parser/Parser.hpp"
#include "../../../mType/services/ImportManager.hpp"
#include "../../../mType/environment/EnvironmentBuilder.hpp"
#include "../../../mType/ast/nodes/statements/ImportNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace mtype::lsp {

namespace {

using ImportSpec = ImportResolver::ImportSpec;

std::string normalizePath(const std::string& path) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(fs::path(path), ec);
    return ec ? path : canonical.string();
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

const ast::nodes::statements::ProgramNode* asProgram(const ast::ASTNode* node) {
    return dynamic_cast<const ast::nodes::statements::ProgramNode*>(node);
}

// The import statements of a parsed file. Multi-line imports and
// `import lib` forms come out right, unlike the line regex below.
std::vector<ImportSpec> collectImportSpecs(const ast::ASTNode* root) {
    std::vector<ImportSpec> specs;
    const auto* program = asProgram(root);
    if (!program) return specs;
    for (const auto& stmt : program->getStatements()) {
        const auto* importNode = dynamic_cast<const ast::nodes::statements::ImportNode*>(stmt.get());
        if (!importNode) continue;
        ImportSpec spec;
        spec.path = importNode->getFilePath();
        spec.library = importNode->isLibraryImport();
        spec.names = importNode->getImportedSymbols();
        // The compiler registers both the original and the alias.
        for (const auto& originalToAlias : importNode->getSymbolAliases()) {
            spec.names.push_back(originalToAlias.second);
        }
        specs.push_back(std::move(spec));
    }
    return specs;
}

// Fallback for a document whose current content doesn't parse: a line
// regex. It misses multi-line and library imports, so callers treat the
// result as incomplete.
std::vector<ImportSpec> regexImportSpecs(const std::string& content) {
    std::vector<ImportSpec> specs;
    static const std::regex importRegex("import\\s+.*\\s+from\\s+\"([^\"]+)\"");
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, importRegex)) {
            ImportSpec spec;
            spec.path = match[1].str();
            specs.push_back(std::move(spec));
        }
    }
    return specs;
}

void collectDeclaredTypeNames(const ast::ASTNode* root,
                              std::unordered_set<std::string>& out) {
    const auto* program = asProgram(root);
    if (!program) return;
    for (const auto& stmt : program->getStatements()) {
        if (const auto* cls = dynamic_cast<const ast::nodes::classes::ClassNode*>(stmt.get())) {
            out.insert(cls->getClassName());
        } else if (const auto* iface =
                       dynamic_cast<const ast::nodes::classes::InterfaceNode*>(stmt.get())) {
            out.insert(iface->getName());
        }
    }
}

// Copies classes, interfaces and functions; a name the target already has
// is kept (registration throws on duplicates, which is ignored).
void registerAll(const environment::Environment& from, environment::Environment& to) {
    if (auto classes = from.getClassRegistry()) {
        for (const auto& name : classes->getAllItemNames()) {
            if (auto def = classes->findClass(name)) {
                try { to.registerClass(name, def); } catch (...) {}
            }
        }
    }
    if (auto interfaces = from.getInterfaceRegistry()) {
        for (const auto& [name, def] : interfaces->getAllInterfaces()) {
            try { to.registerInterface(name, def); } catch (...) {}
        }
    }
    if (auto functions = from.getFunctionRegistry()) {
        for (const auto& name : functions->getAllItemNames()) {
            if (auto def = functions->findFunction(name)) {
                try { to.registerFunction(name, def); } catch (...) {}
            }
        }
    }
}

} // namespace

ImportResolver::ImportResolver() {
}

void ImportResolver::setProjectConfig(std::shared_ptr<ProjectConfigProvider> config) {
    projectConfig_ = std::move(config);
    clearCache();
}

void ImportResolver::clearCache() {
    cache_.clear();
}

ImportSummary ImportResolver::resolveImports(Document* doc, bool astIsCurrent) {
    ImportSummary summary;
    if (!doc || !doc->environment) {
        return summary;
    }

    // A stale AST (kept after a failed parse) must not supply the import
    // list; the regex is all that's left, and it can miss imports.
    bool complete = astIsCurrent && !doc->ast.empty();
    const auto specs = complete ? collectImportSpecs(doc->ast.front().get())
                                : regexImportSpecs(doc->content);

    const std::string docPath = normalizePath(UriUtils::uriToFilePath(doc->uri));
    std::unordered_set<std::string> visited{docPath}; // never import self
    MergeTarget target{doc->environment, doc->symbolLocations, summary.typeNames};
    for (const auto& spec : specs) {
        // No short-circuit: keep merging what does resolve, for completion.
        complete = mergeImport(spec, docPath, target, visited) && complete;
    }
    summary.complete = complete;
    return summary;
}

bool ImportResolver::mergeImport(const ImportSpec& spec, const std::string& fromPath,
                                 MergeTarget& target, std::unordered_set<std::string>& visited) {
    target.typeNames.insert(spec.names.begin(), spec.names.end());
    if (spec.library) {
        return false; // native library: its types are invisible to the LS
    }
    const std::string path = resolveTarget(fromPath, spec.path);
    if (path.empty()) {
        return false;
    }
    return mergeFile(path, target, visited);
}

bool ImportResolver::mergeFile(const std::string& path, MergeTarget& target,
                               std::unordered_set<std::string>& visited) {
    // Mark before recursing so import cycles terminate.
    if (!visited.insert(path).second) {
        return true;
    }
    const FileEntry* entry = loadEntry(path);
    if (!entry) {
        return false; // unreadable or unparsable
    }

    registerAll(*entry->env, *target.env);
    for (const auto& [name, location] : entry->locations) {
        target.locations[name] = location;
    }
    target.typeNames.insert(entry->typeNames.begin(), entry->typeNames.end());

    bool complete = entry->selfComplete;
    for (const auto& nested : entry->nestedPaths) {
        complete = mergeFile(nested, target, visited) && complete;
    }
    return complete;
}

const ImportResolver::FileEntry* ImportResolver::loadEntry(const std::string& path) {
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        return &it->second;
    }
    // Failures aren't cached, so a broken import is retried once fixed.
    auto entry = parseEntry(path);
    if (!entry) {
        return nullptr;
    }
    return &cache_.emplace(path, std::move(*entry)).first->second;
}

std::optional<ImportResolver::FileEntry> ImportResolver::parseEntry(const std::string& path) const {
    std::string content;
    if (!readFile(path, content)) {
        return std::nullopt;
    }
    try {
        auto fileReader = std::make_unique<MemoryFileReader>();
        fileReader->setContent(path, content);
        lexer::Lexer lexer(path, std::move(fileReader));
        parser::Parser parser(lexer, std::make_unique<services::ImportManager>());
        auto program = parser.parseProgram();
        if (!program) {
            return std::nullopt;
        }

        FileEntry entry;
        entry.env = environment::EnvironmentBuilder::createDefault();
        SymbolRegistrationVisitor visitor(entry.env);
        visitor.processProgram(program.get(), path);
        entry.locations = visitor.getSymbolLocations();
        collectDeclaredTypeNames(program.get(), entry.typeNames);

        for (const auto& spec : collectImportSpecs(program.get())) {
            entry.typeNames.insert(spec.names.begin(), spec.names.end());
            const std::string nested = spec.library ? "" : resolveTarget(path, spec.path);
            if (nested.empty()) {
                entry.selfComplete = false;
            } else {
                entry.nestedPaths.push_back(nested);
            }
        }
        return entry;
    } catch (const std::exception&) {
        return std::nullopt; // parse or registration error in the imported file
    }
}

std::string ImportResolver::resolveTarget(const std::string& fromFilePath,
                                          const std::string& importPath) const {
    if (importPath.empty()) {
        return "";
    }
    const std::string baseDir = fs::path(fromFilePath).parent_path().string();
    std::string resolved;
    if (projectConfig_ && projectConfig_->isLoaded()) {
        resolved = projectConfig_->resolveImport(baseDir, importPath);
    }
    if (resolved.empty()) {
        resolved = (fs::path(baseDir) / importPath).string();
    }
    std::error_code ec;
    if (!fs::is_regular_file(fs::path(resolved), ec)) {
        return "";
    }
    return normalizePath(resolved);
}

} // namespace mtype::lsp
