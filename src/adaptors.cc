#include "adaptors.hh"

namespace EpiMy {
	namespace Adaptors {
		std::string stringToFunctor(std::string string) {
			std::string::size_type position = 0;
			while ((position = string.find("'", position)) != std::string::npos) {
				string.replace(position, 1, "\\'");
				position += 2;
			}
			return "'" + string + "'";
		}
		
		std::string functorToString(std::string functor) {
			// Remove enclosing quotes and escaping.
			if (functor.length() > 2 && functor[0] == '\'') {
				functor = functor.substr(1, functor.length() - 2);
				std::string::size_type position = 0;
				while ((position = functor.find("\\'", position)) != std::string::npos) {
					functor.replace(position, 2, "'");
					position += 1;
				}
			}
			return functor;
		}
		
		std::string convertObjToTerm(::MysoreScript::Obj& obj) {
			// Converts a MysoreScript object into an Epilog term.
			// Currently only integers and strings are supported.
			if (::MysoreScript::isInteger(obj)) {
				return std::to_string(::MysoreScript::getInteger(obj));
			} else {
				::MysoreScript::Object& object = *obj;
				std::string className = object.isa->className;
				if (object.isa == &::MysoreScript::StringClass) {
					return stringToFunctor(reinterpret_cast<::MysoreScript::String*>(obj)->characters);
				}
				return stringToFunctor("mysorescript_object (" + className + ")");
			}
		}
		
		::MysoreScript::Obj convertTermToObj(std::string term) {
			// Converts an Epilog term into a MysoreScript object.
			// Currently any term is converted into a string containing the trace of that term.
			term = functorToString(term);
			::MysoreScript::String* string = gcAlloc<::MysoreScript::String>(term.length());
			if (string == nullptr) {
				throw ::Epilog::RuntimeException("Couldn't allocate space for a new MysoreScript string.", __FILENAME__, __func__, __LINE__);
			}
			string->isa = &::MysoreScript::StringClass;
			string->length = ::MysoreScript::createSmallInteger(term.length());
			term.copy(string->characters, term.length());
			return reinterpret_cast<::MysoreScript::Obj>(string);
		}
		
		Epilog::Epilog(Interpreter::Context& epimyContext) {
			// Extend the Epilog standard library to include built-in functions to interface between languages.
			::Epilog::StandardLibrary::functions["var/2"] = [& epimyContext] (::Epilog::Interpreter::Context& epilogContext) {
				pushInstruction(epilogContext, new ::Epilog::CommandInstruction("var"));
				// The functor here, <dynamic_term>, will never actually be matched. The var command directly before it modifies its following instruction and replaces the functor name with the MysoreScript variable value. <dynamic_term> is therefore simply a placeholder.
				pushInstruction(epilogContext, new ::Epilog::UnifyCompoundTermInstruction(::Epilog::HeapFunctor("<dynamic_term>", 0), ::Epilog::HeapReference(::Epilog::StorageArea::reg, 1)));
				pushInstruction(epilogContext, new ::Epilog::ProceedInstruction());
			};
			
			::Epilog::StandardLibrary::commands["var"] = [& epimyContext] () {
				// Because we execute instructions directly after interpretation, the EpiMy context will contain the correct stack at the time the Epilog instructions are executed, meaning we can just use the top of the stack, rather than indexing into the context vector.
				for (auto i = epimyContext.stack.rbegin(); i != epimyContext.stack.rend(); ++ i) {
					// We're only interested in getting variables from MysoreScript contexts.
					if (MysoreScript* adaptor = dynamic_cast<MysoreScript*>((*i).get())) {
						::MysoreScript::Obj* value;
						if ((value = adaptor->context.lookupSymbol(::Epilog::Runtime::registers[0]->trace())) != nullptr) {
							if (::Epilog::UnifyCompoundTermInstruction* instruction = dynamic_cast<::Epilog::UnifyCompoundTermInstruction*>(::Epilog::Runtime::instructions[::Epilog::Runtime::nextInstruction + 1].get())) {
								instruction->functor.name = convertObjToTerm(*value);
								return;
							} else {
								throw ::Epilog::RuntimeException("Tried to bind to a MysoreScript variable without then unifying.", __FILENAME__, __func__, __LINE__);
							}
						} else {
							throw ::Epilog::UnificationError("Tried to bind to an undefined MysoreScript variable.", __FILENAME__, __func__, __LINE__);
						}
					}
				}
				throw ::Epilog::UnificationError("Tried to access an inexistent MysoreScript context.", __FILENAME__, __func__, __LINE__);
			};
			
			::Epilog::StandardLibrary::functions["set/2"] = [& epimyContext] (::Epilog::Interpreter::Context& epilogContext) {
				pushInstruction(epilogContext, new ::Epilog::CommandInstruction("set"));
				pushInstruction(epilogContext, new ::Epilog::ProceedInstruction());
			};
			
			::Epilog::StandardLibrary::commands["set"] = [& epimyContext] () {
				// Because we execute instructions directly after interpretation, the EpiMy context will contain the correct stack at the time the Epilog instructions are executed, meaning we can just use the top of the stack, rather than indexing into the context vector.
				for (auto i = epimyContext.stack.rbegin(); i != epimyContext.stack.rend(); ++ i) {
					// We're only interested in getting variables from MysoreScript contexts.
					if (MysoreScript* adaptor = dynamic_cast<MysoreScript*>((*i).get())) {
						adaptor->context.setSymbol(::Epilog::Runtime::registers[0]->trace(), convertTermToObj(::Epilog::Runtime::registers[1]->trace()));
						return;
					}
				}
				throw ::Epilog::UnificationError("Tried to access an inexistent MysoreScript context.", __FILENAME__, __func__, __LINE__);
			};
		}
		
		void Epilog::execute(std::string string) {
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
		
		MysoreScript::MysoreScript(Interpreter::Context& context) { }

		void MysoreScript::execute(std::string string) {
			std::unique_ptr<::MysoreScript::AST::Statements> root;
			pegmatite::StringInput input(string);
			if (parser.parse(input, parser.g.statements, parser.g.ignored, pegmatite::defaultErrorReporter, root)) {
				root->interpret(context);
			} else {
				std::cerr << "Could not parse the MysoreScript block." << std::endl;
			}
		}
	}
}
