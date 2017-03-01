#include <fstream>
#include <iostream>
#include "interpreter.hh"
#include "Epilog/src/parser.hh"
#include "Epilog/src/runtime.hh"
#include "Epilog/src/standardlibrary.hh"
#include "MysoreScript/src/parser.hh"
#include "MysoreScript/src/interpreter.hh"

#define POLYMORPHIC(class) \
	virtual ~class() = default;\
	class() = default;\
	class(const class&) = default;\
	class& operator=(const class& other) = default;

namespace EpiMy {
	extern pegmatite::ErrorReporter errorReporter;
	
	namespace Adaptors {
		extern Interpreter::Context* currentContext;
		
		class Adaptor {
			public:
			POLYMORPHIC(Adaptor)
			
			virtual void execute(pegmatite::Input& input) = 0;
			virtual void execute(const std::string& string) = 0;
			virtual void execute(std::ifstream stream) = 0;
		};
		
		class Epilog: public Adaptor {
			::Epilog::Parser::EpilogParser parser;
			
			public:
			::Epilog::Interpreter::Context context;
			::Epilog::Runtime runtime;
			
			Epilog(Interpreter::Context&);
			
			virtual void execute(pegmatite::Input& input) override;
			virtual void execute(const std::string& string) override;
			virtual void execute(std::ifstream stream) override;
		};
		
		class MysoreScript: public Adaptor {
			friend class Epilog;
			
			::MysoreScript::Parser::MysoreScriptParser parser;
			// The MysoreScript interpreter uses the AST during interpretation, so we must make sure that the ASTs are not freed between language blocks.
			std::vector<std::unique_ptr<::MysoreScript::AST::Statements>> previousASTs;
			
			public:
			::MysoreScript::Interpreter::Context context;
			
			MysoreScript(Interpreter::Context&);
			
			virtual void execute(pegmatite::Input& input) override;
			virtual void execute(const std::string& string) override;
			virtual void execute(std::ifstream stream) override;
		};
		
		::MysoreScript::Obj convertTermToObj(const ::Epilog::HeapReference& term);
	}
}
