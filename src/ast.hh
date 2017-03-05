#include <iostream>
#include "Pegmatite/ast.hh"
#include "MysoreScript/src/parser.hh"
#include "Epilog/src/parser.hh"
#include "Epilog/src/runtime.hh"
#include "interpreter.hh"

namespace EpiMy {
	namespace Adaptors {
		extern Interpreter::Context* currentContext;
	}
	
	namespace AST {
		using pegmatite::ASTPtr;
		using pegmatite::ASTChild;
		using pegmatite::ASTList;
		using pegmatite::ErrorReporter;
		
		class LanguageIdentifier: public pegmatite::ASTString { };
		
		class LanguageExtract: public pegmatite::ASTString { };
		
		class LanguageBlockExtract: public LanguageExtract { };
		
		class LanguageExpressionExtract: public LanguageExtract { };
		
		class LanguageExpression {
			public:
			pegmatite::ASTChild<LanguageIdentifier> language;
			pegmatite::ASTChild<LanguageExtract> extract;
		};
		
		class LanguageBlock: public pegmatite::ASTContainer {
			public:
			pegmatite::ASTChild<LanguageIdentifier> language;
			pegmatite::ASTChild<LanguageBlockExtract> extract;
		};
		
		class LanguageBlocks: public pegmatite::ASTContainer {
			pegmatite::ASTList<LanguageBlock> blocks;
			public:
			virtual void interpret(Interpreter::Context& context);
		};
	}
}

namespace MysoreScript {
	namespace AST {
		struct LanguageExpression: public Expression, public ::EpiMy::AST::LanguageExpression {
			bool isConstantExpression() override {
				// Not all languages have support for constantness checking, so we just re-evaluate the expression each time to be safe.
				return false;
			}
			
			Obj evaluateExpr(Interpreter::Context& context) override;
			
			llvm::Value* compileExpression(Compiler::Context& context) override {
				throw ::Epilog::CompilationException("Tried to compile a language expression.", __FILENAME__, __func__, __LINE__);
			}
			
			void collectVarUses(std::unordered_set<std::string>& declaration, std::unordered_set<std::string>& uses) override {
				// Language expressions do not behave like normal expressions, and variables may not be captured properly.
				return;
			}
		};
	}
}

namespace Epilog {
	namespace AST {
		class LanguageExpression: public DynamicTerm, public ::EpiMy::AST::LanguageExpression {
			public:
			std::string toString() const override {
				return "@[" + language + "]: " + extract + "@:";
			}
			
			std::list<Instruction*> instructions(std::shared_ptr<TermNode> node, std::unordered_map<std::string, HeapReference>& allocations, bool bodyTerm, bool argumentTerm) const override;
			
			LanguageExpression() : DynamicTerm("<language_expression>") { }
		};
	}
}
