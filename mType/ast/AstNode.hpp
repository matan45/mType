#pragma once
#include "../core/Result.hpp"
#include "../core/Value.hpp"
#include <memory>
#include <vector>

namespace mtype {
	namespace core
	{
		class Environment;
	}

	// Forward declarations
	class Visitor;

	namespace ast {
		// Base class for all AST nodes
		class ASTNode {
		private:
			int line;
			int column;
			std::string sourceFile;

		public:
			explicit ASTNode(int l = -1, int c = -1) : line(l), column(c) {}
			virtual ~ASTNode() = default;

			// Visitor pattern support
			virtual core::Value accept(Visitor* visitor) = 0;

			// Location information
			int getLine() const { return line; }
			int getColumn() const { return column; }
			std::string getSourceFile() const { return sourceFile; }

			void setLocation(int l, int c) { line = l; column = c; }
			void setSourceFile(std::string_view file) { sourceFile = file; }

			// Type checking (for semantic analysis)
			virtual core::Result<core::ValueType> inferType(core::Environment* env) {
				return core::Result<core::ValueType>::err(core::Error::runtime("Type inference not implemented"));
			}

			// Clone for AST transformations
			virtual std::unique_ptr<ASTNode> clone() const = 0;

			// Debug string representation
			virtual std::string toString() const = 0;
		};

		// Helper for managing AST node pointers
		using ASTNodePtr = std::unique_ptr<ASTNode>;
		using ASTNodeList = std::vector<ASTNodePtr>;

		// Base class for visitor pattern
		class Visitor {
		public:
			virtual ~Visitor() = default;

			// Visit methods for each node type
			// These will be declared as nodes are defined
		};
	}
}
