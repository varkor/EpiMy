#include "grammar.hh"
#include "ast.hh"

namespace EpiMy {
	namespace Parser {
		class EpiMyParser: public pegmatite::ASTParserDelegate {
			BindAST<AST::LanguageIdentifier> languageIdentifier = EpiMyGrammar::get().languageIdentifier;
			BindAST<AST::LanguageBlockExtract> languageBlockExtract = EpiMyGrammar::get().languageBlockExtract;
			BindAST<AST::LanguageExpressionExtract> languageExpressionExtract = EpiMyGrammar::get().languageExpressionExtract;
			BindAST<AST::LanguageBlocks> languageBlocks = EpiMyGrammar::get().languageBlocks;
			BindAST<AST::LanguageBlock> languageBlock = EpiMyGrammar::get().languageBlock;
			
			public:
			EpiMyGrammar& grammar = EpiMyGrammar::get();
		};
		
		class EpilogParser: public ::Epilog::Parser::EpilogParser {
			BindAST<AST::LanguageIdentifier> languageIdentifier = EpiMyGrammar::get().languageIdentifier;
			BindAST<AST::LanguageExpressionExtract> languageExpressionExtract = EpiMyGrammar::get().languageExpressionExtract;
			BindAST<::Epilog::AST::LanguageExpression> languageExpression = EpiMyGrammar::get().languageExpression;
		};
		
		class MysoreScriptParser: public ::MysoreScript::Parser::MysoreScriptParser {
			BindAST<AST::LanguageIdentifier> languageIdentifier = EpiMyGrammar::get().languageIdentifier;
			BindAST<AST::LanguageExpressionExtract> languageExpressionExtract = EpiMyGrammar::get().languageExpressionExtract;
			BindAST<::MysoreScript::AST::LanguageExpression> languageExpression = EpiMyGrammar::get().languageExpression;
			
			using ::MysoreScript::Parser::MysoreScriptParser::g; // Make the original grammar private.
			
			public:
			::MysoreScript::Parser::MysoreScriptGrammar& grammar = ::MysoreScript::Parser::MysoreScriptGrammar::get();
		};
	}
}
