#include "adaptors.hh"

namespace EpiMy {
	namespace Interpreter {
		std::shared_ptr<Adaptors::Adaptor> adaptorForLanguage(Interpreter::Context& context, std::string language) {
			if (context.adaptors.find(language) == context.adaptors.end()) {
				if (language == "Epilog") {
					context.adaptors[language] = std::shared_ptr<Adaptors::Adaptor>(new Adaptors::Epilog(context));
				} else if (language == "MysoreScript") {
					context.adaptors[language] = std::shared_ptr<Adaptors::Adaptor>(new Adaptors::MysoreScript(context));
				} else {
					throw ::Epilog::CompilationException("Encountered an unknown language: " + language, __FILENAME__, __func__, __LINE__);
				}
			}
			return context.adaptors[language];
		}
		
		std::shared_ptr<Adaptors::Adaptor> interpretExtract(Interpreter::Context& context, std::string language, std::string extract) {
			Adaptors::currentContext = &context;
			std::shared_ptr<Adaptors::Adaptor> adaptor(adaptorForLanguage(context, language));
			context.stack.push_back(adaptor);
			return adaptor;
		}
		
		void executeExtract(Interpreter::Context& context, std::string language, std::string extract) {
			std::shared_ptr<Adaptors::Adaptor> adaptor(interpretExtract(context, language, extract));
			adaptor->execute(extract);
		}
		
		std::shared_ptr<void> evaluateExtract(Interpreter::Context& context, std::string language, std::string extract, std::string target) {
			std::shared_ptr<Adaptors::Adaptor> adaptor(interpretExtract(context, language, extract));
			std::shared_ptr<void> result(adaptor->evaluate(extract));
			return Adaptors::Adaptor::convertValue(result, language, target);
		}
	}
	
	namespace AST {
		void LanguageBlocks::interpret(Interpreter::Context& context) {
			Adaptors::currentContext = &context;
			for (std::unique_ptr<LanguageBlock>& block : blocks) {
				executeExtract(context, block->language, block->extract);
			}
		}
	}
}
