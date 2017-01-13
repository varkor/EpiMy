#include "adaptors.hh"
#include "parser.hh"

namespace EpiMy {
	namespace AST {
		void LanguageBlocks::interpret(Interpreter::Context& context) {
			for (std::unique_ptr<LanguageBlock>& block : blocks) {
				if (block->language == "Epilog") {
					Adaptors::Epilog adaptor;
					adaptor.execute(block->extract);
				} else if (block->language == "MysoreScript") {
					Adaptors::MysoreScript adaptor;
					adaptor.execute(block->extract);
				} else {
					std::cerr << "Encountered an unknown language: " << block->language << std::endl;
				}
			}
		}
	}
}
