#include "Epilog/src/parser.hh"
#include "Epilog/src/runtime.hh"
#include "MysoreScript/src/parser.hh"
#include "MysoreScript/src/interpreter.hh"
#include "adaptors.hh"

void EpiMy::Adaptors::Epilog::execute(std::string string) {
	::Epilog::Parser::EpilogParser parser;
	::Epilog::Interpreter::Context context;
	std::unique_ptr<::Epilog::AST::Clauses> root;
	pegmatite::StringInput input(string);
	if (parser.parse(input, parser.grammar.clauses, parser.grammar.ignored, pegmatite::defaultErrorReporter, root)) {
		try {
			root->interpret(context);
			std::cout << "true." << std::endl;
		} catch (const ::Epilog::UnificationError& error) {
			std::cout << "false." << std::endl;
		} catch (const ::Epilog::RuntimeException& exception) {
			exception.print();
		}
	} else {
		std::cerr << "Could not parse the Epilog block." << std::endl;
	}
}

void EpiMy::Adaptors::MysoreScript::execute(std::string string) {
	::MysoreScript::Parser::MysoreScriptParser parser;
	::MysoreScript::Interpreter::Context context;
	std::unique_ptr<::MysoreScript::AST::Statements> root;
	pegmatite::StringInput input(string);
	if (parser.parse(input, parser.g.statements, parser.g.ignored, pegmatite::defaultErrorReporter, root)) {
		root->interpret(context);
	} else {
		std::cerr << "Could not parse the MysoreScript block." << std::endl;
	}
}
