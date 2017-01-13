#include <fcntl.h>
#include <gc.h>
#include "adaptors.hh"
#include "parser.hh"

void usage(const char command[]) {
	std::cerr << "usage: " << command << " <file>" << std::endl;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage(argv[0]);
		
		return EXIT_FAILURE;
	} else {
		GC_init();
		EpiMy::Parser::EpiMyParser parser;
		EpiMy::Interpreter::Context context;
		std::unique_ptr<EpiMy::AST::LanguageBlocks> root;
		pegmatite::AsciiFileInput input(open(argv[1], O_RDONLY));
		if (parser.parse(input, parser.grammar.languageBlocks, parser.grammar.ignored, pegmatite::defaultErrorReporter, root)) {
			root->interpret(context);
			return EXIT_SUCCESS;
		} else {
			return EXIT_FAILURE;
		}
	}
}
