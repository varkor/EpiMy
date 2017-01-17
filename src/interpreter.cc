#include "adaptors.hh"
#include "parser.hh"

namespace EpiMy {
	namespace AST {
		void LanguageBlocks::interpret(Interpreter::Context& context) {
			for (std::unique_ptr<LanguageBlock>& block : blocks) {
				if (context.adaptors.find(block->language) == context.adaptors.end()) {
					if (block->language == "Epilog") {
						context.adaptors[block->language] = std::shared_ptr<Adaptors::Adaptor>(new Adaptors::Epilog(context));
					} else if (block->language == "MysoreScript") {
						context.adaptors[block->language] = std::shared_ptr<Adaptors::Adaptor>(new Adaptors::MysoreScript(context));
					} else {
						throw ::Epilog::CompilationException("Encountered an unknown language: " + block->language, __FILENAME__, __func__, __LINE__);
					}
				}
				context.stack.push_back(context.adaptors[block->language]);
				context.adaptors[block->language]->execute(block->extract);
			}
		}
	}
}
