#include "adaptors.hh"

namespace EpiMy {
	namespace Adaptors {
		std::string escapeFunctorName(std::string string) {
			std::string::size_type position = 0;
			while ((position = string.find("'", position)) != std::string::npos) {
				string.replace(position, 1, "\\'");
				position += 2;
			}
			return "'" + string + "'";
		}
		
		::Epilog::HeapReference pushCompoundTerm(::Epilog::HeapFunctor const& functor) {
			::Epilog::HeapReference::heapIndex index = ::Epilog::Runtime::heap.size();
			::Epilog::HeapTuple header(::Epilog::HeapTuple::Type::compoundTerm, index + 1);
			::Epilog::Runtime::heap.push_back(header.copy());
			::Epilog::Runtime::heap.push_back(functor.copy());
			return ::Epilog::HeapReference(::Epilog::StorageArea::heap, index);
		}
		
		void pushParameterReference(::Epilog::HeapReference reference) {
			if (::Epilog::HeapTuple* tuple = dynamic_cast<::Epilog::HeapTuple*>(::Epilog::dereference(reference).getPointer())) {
				// We need to point at the functor rather than the compound term tuple.
				reference.index = tuple->reference;
			}
			::Epilog::Runtime::heap.push_back(::Epilog::HeapTuple(::Epilog::HeapTuple::Type::compoundTerm, reference.index).copy());
		}
		
		::Epilog::HeapReference convertObjToTerm(::MysoreScript::Obj& obj) {
			// Converts a MysoreScript object into an Epilog term.
			// Currently only integers and strings are supported.
			if (::MysoreScript::isInteger(obj)) {
				::Epilog::HeapReference::heapIndex index = ::Epilog::Runtime::heap.size();
				::Epilog::HeapNumber number(::MysoreScript::getInteger(obj));
				::Epilog::Runtime::heap.push_back(number.copy());
				return ::Epilog::HeapReference(::Epilog::StorageArea::heap, index);
			} else {
				::MysoreScript::Object& object = *obj;
				std::string className = object.isa->className;
				std::string functorName = "<mysorescript_object(" + className + ")>";
				if (object.isa == &::MysoreScript::StringClass) {
					functorName = reinterpret_cast<::MysoreScript::String*>(obj)->characters;
				}
				if (object.isa == &::MysoreScript::ArrayClass) {
					::MysoreScript::Array* array = reinterpret_cast<::MysoreScript::Array*>(obj);
					intptr_t length = array->length ? ::MysoreScript::getInteger(array->length) : 0;
					::Epilog::HeapReference previous = pushCompoundTerm(::Epilog::HeapFunctor("[]", 0));
					for (intptr_t i = length - 1; i >= 0; -- i) {
						bool inPlace = ::MysoreScript::isInteger(array->buffer[i]);
						::Epilog::HeapReference element;
						if (!inPlace) {
							element = convertObjToTerm(array->buffer[i]);
						}
						::Epilog::HeapReference current(pushCompoundTerm(::Epilog::HeapFunctor(".", 2)));
						if (!inPlace) {
							pushParameterReference(element);
						} else {
							convertObjToTerm(array->buffer[i]);
						}
						pushParameterReference(previous);
						previous = current;
					}
					return previous;
				}
				return pushCompoundTerm(::Epilog::HeapFunctor(escapeFunctorName(functorName), 0));
			}
		}
		
		bool buildArray(std::list<::MysoreScript::Obj>& elements, ::Epilog::HeapContainer* term) {
			// Returns whether the term was a true list.
			if (::Epilog::HeapTuple* tuple = dynamic_cast<::Epilog::HeapTuple*>(term)) {
				::Epilog::HeapFunctor* functor;
				if (tuple->type == ::Epilog::HeapTuple::Type::compoundTerm && (functor = dynamic_cast<::Epilog::HeapFunctor*>(::Epilog::Runtime::heap[tuple->reference].get()))) {
					std::string symbol = functor->toString();
					if (symbol == "./2") {
						elements.push_back(convertTermToObj(::Epilog::Runtime::heap[tuple->reference + 1].get()));
						return buildArray(elements, ::Epilog::Runtime::heap[tuple->reference + 2].get());
					} else if (symbol == "[]/0") {
						return true;
					} else {
						return false;
					}
				}
			}
			return false;
		}
		
		::MysoreScript::Obj convertTermToObj(::Epilog::HeapContainer* term) {
			// Converts an Epilog term into a MysoreScript object.
			// Epilog lists correspond to MysoreScript arrays, integers correspond to integers, and anything other compound term is converted into a string.
			std::list<::MysoreScript::Obj> elements;
			if (buildArray(elements, term)) {
				return ::MysoreScript::constructArrayObj(elements);
			}
			if (::Epilog::HeapNumber* number = dynamic_cast<::Epilog::HeapNumber*>(term)) {
				return ::MysoreScript::createSmallInteger(number->value);
			}
			return ::MysoreScript::constructStringObj(term->trace());
		}
		
		Epilog::Epilog(Interpreter::Context& epimyContext) {
			// Extend the Epilog standard library to include built-in functions to interface between languages.
			::Epilog::StandardLibrary::functions["var/2"] = [& epimyContext] (::Epilog::Interpreter::Context& epilogContext, ::Epilog::HeapReference::heapIndex& registers) {
				pushInstruction(epilogContext, new ::Epilog::CommandInstruction("var"));
				pushInstruction(epilogContext, new ::Epilog::UnifyRegisterAndArgumentInstruction(::Epilog::HeapReference(::Epilog::StorageArea::reg, 2), ::Epilog::HeapReference(::Epilog::StorageArea::reg, 1)));
				pushInstruction(epilogContext, new ::Epilog::ProceedInstruction());
				registers = 3;
			};
			
			::Epilog::StandardLibrary::commands["var"] = [& epimyContext] () {
				// Because we execute instructions directly after interpretation, the EpiMy context will contain the correct stack at the time the Epilog instructions are executed, meaning we can just use the top of the stack, rather than indexing into the context vector.
				for (auto i = epimyContext.stack.rbegin(); i != epimyContext.stack.rend(); ++ i) {
					// We're only interested in getting variables from MysoreScript contexts.
					if (MysoreScript* adaptor = dynamic_cast<MysoreScript*>((*i).get())) {
						::MysoreScript::Obj* value;
						if ((value = adaptor->context.lookupSymbol(::Epilog::Runtime::registers[0]->trace())) != nullptr) {
							if (::Epilog::UnifyRegisterAndArgumentInstruction* instruction = dynamic_cast<::Epilog::UnifyRegisterAndArgumentInstruction*>(::Epilog::Runtime::instructions[::Epilog::Runtime::nextInstruction + 1].get())) {
								instruction->registerReference.assign(convertObjToTerm(*value).getAsCopy());
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
			
			::Epilog::StandardLibrary::functions["set/2"] = [& epimyContext] (::Epilog::Interpreter::Context& epilogContext, ::Epilog::HeapReference::heapIndex& registers) {
				pushInstruction(epilogContext, new ::Epilog::CommandInstruction("set"));
				pushInstruction(epilogContext, new ::Epilog::ProceedInstruction());
			};
			
			::Epilog::StandardLibrary::commands["set"] = [& epimyContext] () {
				// Because we execute instructions directly after interpretation, the EpiMy context will contain the correct stack at the time the Epilog instructions are executed, meaning we can just use the top of the stack, rather than indexing into the context vector.
				for (auto i = epimyContext.stack.rbegin(); i != epimyContext.stack.rend(); ++ i) {
					// We're only interested in getting variables from MysoreScript contexts.
					if (MysoreScript* adaptor = dynamic_cast<MysoreScript*>((*i).get())) {
						adaptor->context.setSymbol(::Epilog::Runtime::registers[0]->trace(), convertTermToObj(::Epilog::dereference(::Epilog::HeapReference(::Epilog::StorageArea::reg, 1)).getPointer()));
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
				} catch (const ::Epilog::UnificationError& error) { 
					// Could not unify. In a standalone context, this would print "false.", but inside EpiMy, it makes more sense simply to display nothing.
				} catch (const ::Epilog::RuntimeException& exception) {
					exception.print();
				}
			} else {
				throw ::Epilog::CompilationException("Could not parse the Epilog block.", __FILENAME__, __func__, __LINE__);
			}
		}
		
		MysoreScript::MysoreScript(Interpreter::Context& context) { }

		void MysoreScript::execute(std::string string) {
			std::unique_ptr<::MysoreScript::AST::Statements> root;
			pegmatite::StringInput input(string);
			if (parser.parse(input, parser.g.statements, parser.g.ignored, pegmatite::defaultErrorReporter, root)) {
				root->interpret(context);
			} else {
				throw ::Epilog::CompilationException("Could not parse the MysoreScript block.", __FILENAME__, __func__, __LINE__);
			}
		}
	}
}
