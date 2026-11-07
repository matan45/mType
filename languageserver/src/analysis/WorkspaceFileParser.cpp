#include "WorkspaceFileParser.hpp"

#include "../DocumentManager.hpp"  // full definition of SymbolLocationInfo
#include "../utils/MemoryFileReader.hpp"
#include "SymbolRegistrationVisitor.hpp"
#include "TypeGraphExtractor.hpp"

#include "../../../mType/lexer/Lexer.hpp"
#include "../../../mType/parser/Parser.hpp"
#include "../../../mType/services/ImportManager.hpp"
#include "../../../mType/environment/EnvironmentBuilder.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/optimizer/passes/LombokSynthesisPass.hpp"
#include "../../../mType/optimizer/base/OptimizationContext.hpp"
#include "../../../mType/optimizer/OptimizationConfig.hpp"
#include "../../../mType/token/TokenType.hpp"

#include <iostream>
#include <memory>
#include <utility>

namespace mtype::lsp::analysis
{
    namespace
    {
        // SymbolRegistrationVisitor builds qualified names like "Foo.method"
        // for class members. The symbol tables only keep top-level names;
        // filter the rest.
        bool isTopLevelKey(const std::string& key)
        {
            return key.find('.') == std::string::npos;
        }

        // Best-effort kind detection by inspecting top-level program
        // children. The visitor doesn't return kind, so we walk the
        // ProgramNode children directly.
        WorkspaceSymbolKind detectKindFromAst(ast::ASTNode* program, const std::string& name)
        {
            auto* prog = dynamic_cast<ast::ProgramNode*>(program);
            if (!prog) return WorkspaceSymbolKind::Unknown;
            for (const auto& stmt : prog->getStatements())
            {
                if (auto* cls = dynamic_cast<ast::ClassNode*>(stmt.get()))
                {
                    if (cls->getClassName() == name) return WorkspaceSymbolKind::Class;
                }
                if (auto* iface = dynamic_cast<ast::InterfaceNode*>(stmt.get()))
                {
                    if (iface->getName() == name) return WorkspaceSymbolKind::Interface;
                }
                if (auto* fn = dynamic_cast<ast::FunctionNode*>(stmt.get()))
                {
                    if (fn->getName() == name) return WorkspaceSymbolKind::Function;
                }
            }
            return WorkspaceSymbolKind::Unknown;
        }

        // Throws on any syntax error: the parser has no error recovery.
        std::unique_ptr<ast::ASTNode> parseProgram(const std::string& fileUri,
                                                   const std::string& content)
        {
            auto memReader = std::make_unique<MemoryFileReader>();
            memReader->setContent(fileUri, content);

            lexer::Lexer lex(fileUri, std::move(memReader));
            auto importMgr = std::make_unique<services::ImportManager>();
            parser::Parser parser(lex, std::move(importMgr));
            return parser.parseProgram();
        }

        std::vector<WorkspaceSymbol> collectSymbols(ast::ASTNode* program,
                                                    const std::string& fileUri)
        {
            auto env = environment::EnvironmentBuilder::createDefault();
            SymbolRegistrationVisitor visitor(env);
            visitor.processProgram(program, fileUri);

            std::vector<WorkspaceSymbol> symbols;
            for (const auto& [key, info] : visitor.getSymbolLocations())
            {
                if (!isTopLevelKey(key)) continue;

                WorkspaceSymbol sym;
                sym.name = key;
                sym.fileUri = fileUri;
                sym.line = info.line;
                sym.column = info.column;
                sym.kind = detectKindFromAst(program, key);
                symbols.push_back(std::move(sym));
            }
            return symbols;
        }

        // DocumentManager runs this one optimizer pass so the members it
        // generates (@Getter/@Setter accessors, toString, ...) are visible.
        // Mirror it so a closed file exposes what an open one does
        // (DocumentManager::parseDocument).
        std::unique_ptr<ast::ASTNode> applyLombok(std::unique_ptr<ast::ASTNode> program)
        {
            auto env = environment::EnvironmentBuilder::createDefault();
            optimizer::OptimizationConfig config;
            optimizer::base::OptimizationContext context(env, config);
            optimizer::passes::LombokSynthesisPass pass;
            return pass.optimize(std::move(program), context);
        }

        // The pass consumes the AST, so after a throw the file is parsed
        // again and indexed without generated members.
        std::unique_ptr<ast::ASTNode> withLombok(std::unique_ptr<ast::ASTNode> program,
                                                 const std::string& fileUri,
                                                 const std::string& content)
        {
            try
            {
                return applyLombok(std::move(program));
            }
            catch (const std::exception& e)
            {
                std::cerr << "[mType LSP] Lombok synthesis failed for "
                          << fileUri << ": " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "[mType LSP] Lombok synthesis failed for "
                          << fileUri << ": unknown exception" << std::endl;
            }
            return parseProgram(fileUri, content);
        }

        // The parser keeps only a short lookahead, so ranges need the token
        // list lexed again from the same content, as DocumentManager does.
        // Best-effort: a lexer error keeps the tokens read so far. Only the
        // tokens' type, length and location are used afterwards; their
        // string views die with the lexer.
        std::vector<token::Token> lexTokens(const std::string& fileUri,
                                            const std::string& content)
        {
            std::vector<token::Token> tokens;
            try
            {
                auto reader = std::make_unique<MemoryFileReader>();
                reader->setContent(fileUri, content);
                lexer::Lexer lex(fileUri, std::move(reader));

                token::Token tok;
                do
                {
                    tok = lex.getNextToken();
                    tokens.push_back(tok);
                } while (tok.type != token::TokenType::END);
            }
            catch (const std::exception&)
            {
                // keep what was lexed; ranges degrade to the fallbacks
            }
            return tokens;
        }
    } // namespace

    std::optional<ParsedWorkspaceFile> WorkspaceFileParser::parse(const std::string& fileUri,
                                                                  const std::string& content)
    {
        if (fileUri.empty()) return std::nullopt;

        try
        {
            auto program = parseProgram(fileUri, content);
            if (!program) return std::nullopt;

            ParsedWorkspaceFile parsed;
            parsed.symbols = collectSymbols(program.get(), fileUri); // raw AST, before Lombok
            program = withLombok(std::move(program), fileUri, content);
            const auto tokens = lexTokens(fileUri, content);
            parsed.types = TypeGraphExtractor::extract(program.get(), content, tokens, fileUri);
            return parsed;
        }
        catch (const std::exception& e)
        {
            // Log so indexing failures are diagnosable. Don't propagate: one
            // bad file shouldn't bring down the whole index, and the user sees
            // proper diagnostics for the file when they open it.
            std::cerr << "[mType LSP] Workspace index parse failed for "
                      << fileUri << ": " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[mType LSP] Workspace index parse failed for "
                      << fileUri << ": unknown exception" << std::endl;
        }
        return std::nullopt;
    }
}
