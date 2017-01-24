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
	namespace Adaptors {
		class Adaptor {
			public:
			POLYMORPHIC(Adaptor)
			
			virtual void execute(std::string string) = 0;
		};
		
		class Epilog: public Adaptor {
			::Epilog::Parser::EpilogParser parser;
			::Epilog::Interpreter::Context context;
			
			public:
			Epilog(Interpreter::Context&);
			
			virtual void execute(std::string string) override;
		};
		
		class MysoreScript: public Adaptor {
			friend class Epilog;
			
			::MysoreScript::Parser::MysoreScriptParser parser;
			::MysoreScript::Interpreter::Context context;
			
			public:
			MysoreScript(Interpreter::Context&);
			virtual void execute(std::string string) override;
		};
		
		::MysoreScript::Obj convertTermToObj(::Epilog::HeapContainer* term);
	}
}
