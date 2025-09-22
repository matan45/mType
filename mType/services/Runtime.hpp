#pragma once
#include <string>

// Forward declarations
namespace ast { class ASTNode; }
namespace evaluator { class Evaluator; }

namespace services
{
    /**
     * Isolated execution runtime
     * Creates fresh environment for each execution to prevent state pollution
     */
    class Runtime
    {
    public:
        Runtime() = default;
        ~Runtime() = default;

        /**
         * Execute a compiled AST file in an isolated environment
         * Each execution gets a completely fresh environment
         * @param compiledFile Path to .mtc compiled file
         * @return true if execution successful, false otherwise
         */
        bool execute(const std::string& compiledFile);

    private:
        // No member variables - each execution is isolated
        void preRegisterClassDefinitions(ast::ASTNode* node, evaluator::Evaluator* evaluator);
    };
}