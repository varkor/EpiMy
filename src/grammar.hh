#include "Pegmatite/pegmatite.hh"
#include "Epilog/src/grammar.hh"
#include "MysoreScript/src/grammar.hh"

namespace EpiMy {
	namespace Parser {
		using pegmatite::Rule;
		using pegmatite::operator""_E;
		using pegmatite::operator""_S;
		using pegmatite::ExprPtr;
		using pegmatite::BindAST;
		using pegmatite::any;
		using pegmatite::nl;
		using pegmatite::term;
		
		struct EpiMyGrammar {
			// Whitespace: spaces, tabs and newline characters.
			Rule whitespace = ' '_E | '\t' | nl('\n');
			
			// Line comments.
			Rule comment = '#'_E >> *(!ExprPtr('\n') >> any()) >> nl('\n');
			
			Rule ignored = *(comment | whitespace);
			
			// Digits: 0 to 9 inclusive.
			ExprPtr digit = '0'_E - '9';
			
			// Letters: a to z and A to Z.
			ExprPtr lowercase = 'a'_E - 'z';
			
			ExprPtr uppercase = 'A'_E - 'Z';
			
			ExprPtr letter = lowercase | uppercase;
			
			// Characters: digits, letters and underscores.
			ExprPtr character = letter | digit | '_';
					
			// Language identifier: the name of a programming language.
			Rule languageIdentifier = +character;
			
			// Language expression: an expression in another programming language;
			Rule languageExpressionExtract = *(!ExprPtr("@:") >> any());
			
			Rule languageExpression = term("@["_E >> languageIdentifier >> "]:") >> languageExpressionExtract >> "@:";
			
			Rule languageBlockExtract = *(!ExprPtr("@}") >> any());
			
			// Language block: a block of code from a programming language.
			Rule languageBlock = term("@["_E >> languageIdentifier >> ']') >> '{' >> languageBlockExtract >> "@}";
			
			Rule languageBlocks = *languageBlock;
			
			// Singleton getter.
			static EpiMyGrammar& get() {
				static EpiMyGrammar grammar;
				return grammar;
			}
			
			// Avoid the possibility of accidental copying of the singleton.
			EpiMyGrammar(EpiMyGrammar const&) = delete;
			void operator=(EpiMyGrammar const&) = delete;
			
			// EpilogGrammar should only be constructed via the getter.
			protected:
			EpiMyGrammar() {}
		};
	}
}
