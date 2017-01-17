#include "grammar.hh"
#include "ast.hh"

namespace EpiMy {
	namespace Parser {
		class EpiMyParser: public pegmatite::ASTParserDelegate {
			BindAST<AST::LanguageIdentifier> languageIdentifier = EpiMyGrammar::get().languageIdentifier;
			BindAST<AST::LanguageExtract> languageExtract = EpiMyGrammar::get().languageExtract;
			BindAST<AST::LanguageBlocks> languageBlocks = EpiMyGrammar::get().languageBlocks;
			BindAST<AST::LanguageBlock> languageBlock = EpiMyGrammar::get().languageBlock;
			public:
			const EpiMyGrammar& grammar = EpiMyGrammar::get();
		};
	}
}
