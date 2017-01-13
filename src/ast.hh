#include <iostream>
#include "Pegmatite/ast.hh"
#include "interpreter.hh"

namespace EpiMy {
	namespace AST {
		using pegmatite::ASTPtr;
		using pegmatite::ASTChild;
		using pegmatite::ASTList;
		using pegmatite::ErrorReporter;
		
		class LanguageIdentifier: public pegmatite::ASTString { };
		
		class LanguageExtract: public pegmatite::ASTString { };
		
		class LanguageBlock: public pegmatite::ASTContainer {
			public:
			pegmatite::ASTChild<LanguageIdentifier> language;
			pegmatite::ASTChild<LanguageExtract> extract;
		};
		
		class LanguageBlocks: public pegmatite::ASTContainer {
			pegmatite::ASTList<LanguageBlock> blocks;
			public:
			virtual void interpret(Interpreter::Context& context);
		};
	}
}
