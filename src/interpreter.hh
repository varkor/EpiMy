#include <vector>
#include <unordered_map>
#pragma once

namespace EpiMy {
	namespace Adaptors {
		class Adaptor;
	}
	
	namespace Interpreter {
		class Context {
			public:
			std::unordered_map<std::string, std::shared_ptr<Adaptors::Adaptor>> adaptors;
			std::vector<std::shared_ptr<Adaptors::Adaptor>> stack;
		};
		
		void executeExtract(Interpreter::Context& context, std::string language, std::string extract);
		std::shared_ptr<void> evaluateExtract(Interpreter::Context& context, std::string source, std::string extract, std::string target);
	}
}
