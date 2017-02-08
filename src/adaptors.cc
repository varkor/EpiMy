#include "adaptors.hh"

namespace EpiMy {
	void epimyErrorReporter(const pegmatite::InputRange& inputRange, const std::string& message) {
		std::cerr << "Syntax error at " << inputRange.start.line << ":" << inputRange.start.col << ": " << inputRange.str() << std::endl;
	}
	pegmatite::ErrorReporter errorReporter = epimyErrorReporter;
	
	namespace Adaptors {
		Interpreter::Context* currentContext;
		
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
			// Currently only integers, strings and arrays are supported.
			if (obj == nullptr) {
				throw ::Epilog::RuntimeException("Tried to convert a null object into a term.", __FILENAME__, __func__, __LINE__);
			}
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
				return pushCompoundTerm(::Epilog::HeapFunctor(::Epilog::AST::normaliseIdentifierName(escapeFunctorName(functorName)), 0));
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
			// Converts an Epilog term into a MysoreScript object. This assumes the term has already been dereferenced.
			// Epilog lists correspond to MysoreScript arrays, integers correspond to integers, and any other compound term is converted into a string. Unbound variables are converted into null values.
			if (::Epilog::HeapTuple* tuple = dynamic_cast<::Epilog::HeapTuple*>(term)) {
				if (tuple->type == ::Epilog::HeapTuple::Type::reference) {
					// Terms that are still references after being dereferenced are unbound variables. These are mapped to null values in MysoreScript.
					return nullptr;
				}
				std::list<::MysoreScript::Obj> elements;
				if (buildArray(elements, term)) {
					return ::MysoreScript::constructArrayObj(elements);
				}
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
			
			::Epilog::StandardLibrary::functions["call/3"] = [& epimyContext] (::Epilog::Interpreter::Context& epilogContext, ::Epilog::HeapReference::heapIndex& registers) {
				pushInstruction(epilogContext, new ::Epilog::CommandInstruction("call"));
				pushInstruction(epilogContext, new ::Epilog::UnifyRegisterAndArgumentInstruction(::Epilog::HeapReference(::Epilog::StorageArea::reg, 3), ::Epilog::HeapReference(::Epilog::StorageArea::reg, 2)));
				pushInstruction(epilogContext, new ::Epilog::ProceedInstruction());
				registers = 4;
			};
			
			::Epilog::StandardLibrary::commands["call"] = [& epimyContext] () {
				for (auto i = epimyContext.stack.rbegin(); i != epimyContext.stack.rend(); ++ i) {
					// We're only interested in getting variables from MysoreScript contexts.
					if (MysoreScript* adaptor = dynamic_cast<MysoreScript*>((*i).get())) {
						::MysoreScript::Obj* value;
						if ((value = adaptor->context.lookupSymbol(::Epilog::Runtime::registers[0]->trace())) != nullptr) {
							::MysoreScript::Obj obj = *value;
							if (!::MysoreScript::isInteger(*value) && obj->isa == &::MysoreScript::ClosureClass) {
								::MysoreScript::Closure* closure = reinterpret_cast<::MysoreScript::Closure*>(obj);
								::MysoreScript::Obj argumentObj = convertTermToObj(::Epilog::Runtime::registers[1].get());
								if (!::MysoreScript::isInteger(argumentObj) && argumentObj->isa == &::MysoreScript::ArrayClass) {
									::MysoreScript::Array* array = reinterpret_cast<::MysoreScript::Array*>(argumentObj);
									int arity = array->length ? ::MysoreScript::getInteger(array->length) : 0;
									int arityCheck = ::MysoreScript::isInteger(closure->parameters) ? ::MysoreScript::getInteger(closure->parameters) : 0;
									if (arity != arityCheck) {
										throw ::Epilog::RuntimeException("Tried to call a MysoreScript with the incorrect number of arguments.", __FILENAME__, __func__, __LINE__);
									}
									::MysoreScript::Obj* arguments = new ::MysoreScript::Obj[arity];
									for (int i = 0; i < arity; ++ i) {
										arguments[i] = array->buffer[i];
									}
									::MysoreScript::currentContext = &adaptor->context;
									::MysoreScript::Obj returnValue = ::MysoreScript::callCompiledClosure(closure->invoke, closure, arguments, arity);
									delete[] arguments;
									if (::Epilog::UnifyRegisterAndArgumentInstruction* instruction = dynamic_cast<::Epilog::UnifyRegisterAndArgumentInstruction*>(::Epilog::Runtime::instructions[::Epilog::Runtime::nextInstruction + 1].get())) {
										instruction->registerReference.assign(convertObjToTerm(returnValue).getAsCopy());
										return;
									} else {
										throw ::Epilog::RuntimeException("Tried to bind to a MysoreScript variable without then unifying.", __FILENAME__, __func__, __LINE__);
									}
								} else {
									throw ::Epilog::RuntimeException("Tried to call a MysoreScript closure with a set of arguments that was not a list.", __FILENAME__, __func__, __LINE__);
								}
							} else {
								throw ::Epilog::RuntimeException("Tried to call an non-closure MysoreScript variable.", __FILENAME__, __func__, __LINE__);
							}
						} else {
							throw ::Epilog::RuntimeException("Tried to call an undefined MysoreScript variable.", __FILENAME__, __func__, __LINE__);
						}
					}
				}
				throw ::Epilog::UnificationError("Tried to access an inexistent MysoreScript context.", __FILENAME__, __func__, __LINE__);
			};
		}
		
		void Epilog::execute(std::string string) {
			std::unique_ptr<::Epilog::AST::Clauses> root;
			pegmatite::StringInput input(string);
			if (parser.parse(input, parser.grammar.clauses, parser.grammar.ignored, errorReporter, root)) {
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
		
		::MysoreScript::Obj unify(::MysoreScript::Closure* closure, ::MysoreScript::Obj name, ::MysoreScript::Obj parameters) {
			// This function does not go through the usual execution path for MysoreScript closures, as it relies heavily on manipulating state outside of the MysoreScript runtime's control. Instead, this function acts as a hook that is called whenever the unify() MysoreScript function is called. This does, however, mean that currently this function must be interpreted, and not compiled.
			if (name->isa != &::MysoreScript::StringClass) {
				throw ::Epilog::RuntimeException("Tried to unify an Epilog clause with a non-string functor.", __FILENAME__, __func__, __LINE__);
			}
			if (parameters->isa != &::MysoreScript::ArrayClass) {
				throw ::Epilog::RuntimeException("Tried to unify an Epilog clause with a non-list set of parameters.", __FILENAME__, __func__, __LINE__);
			}
			if (currentContext == nullptr) {
				throw ::Epilog::RuntimeException("Cannot identify current EpiMy context.", __FILENAME__, __func__, __LINE__);
			}
			Interpreter::Context& epimyContext(*currentContext);
			for (auto i = epimyContext.stack.rbegin(); i != epimyContext.stack.rend(); ++ i) {
				// We're only interested in unifying in Epilog contexts.
				if (Epilog* adaptor = dynamic_cast<Epilog*>((*i).get())) {
					// Although we can handle most of the execution process ourselves, in order to actually perform unification, we need to actually push a call instruction to the instruction list as Epilog does not currently support running instructions from a separate instruction vector from the main runtime, and requires an instruction index from which to begin.
					// First, push the arguments to the heap.
					::MysoreScript::String* string = reinterpret_cast<::MysoreScript::String*>(name);
					::MysoreScript::Array* array = reinterpret_cast<::MysoreScript::Array*>(parameters);
					::Epilog::HeapReference::heapIndex length = array->length ? ::MysoreScript::getInteger(array->length) : 0;
					// Make sure we don't overflow the number of Epilog registers.
					while (::Epilog::Runtime::registers.size() < length) {
						::Epilog::Runtime::registers.push_back(nullptr);
					}
					for (::Epilog::HeapReference::heapIndex i = 0; i < length; ++ i) {
						// We can skip the usual put_structure instructions by directly modifying the registers. This is because we're in a MysoreScript context, so we're not going to be interfering with any existing instruction execution by doing so, and we're going to be triggering execution of the instructions immediately following register assignment.
						if (array->buffer[i] != nullptr) {
							::Epilog::Runtime::registers[i] = convertObjToTerm(array->buffer[i]).getAsCopy();
						} else {
							// Null objects refer to parameters that we wish to bind to temporary variables, which will be returned if the unification succeeds.
							::Epilog::HeapTuple header(::Epilog::HeapTuple::Type::reference, ::Epilog::Runtime::heap.size());
							::Epilog::Runtime::heap.push_back(header.copy());
							::Epilog::Runtime::registers[i] = header.copy();
						}
					}
					// Now, call the corresponding functor.
					auto startAddress = ::Epilog::pushInstruction(adaptor->context, new ::Epilog::AllocateInstruction(0));
					::Epilog::pushInstruction(adaptor->context, new ::Epilog::CallInstruction(::Epilog::HeapFunctor(string->characters, length)));
					::Epilog::pushInstruction(adaptor->context, new ::Epilog::DeallocateInstruction());
					try {
						::Epilog::AST::executeInstructions(startAddress, nullptr);
						std::list<::MysoreScript::Obj> bindings;
						for (::Epilog::HeapReference::heapIndex i = 0; i < length; ++ i) {
							bindings.push_back(convertTermToObj(::Epilog::dereference(::Epilog::HeapReference(::Epilog::StorageArea::reg, i)).getPointer()));
						}
						return ::MysoreScript::constructArrayObj(bindings);
					} catch (const ::Epilog::UnificationError&) {
						return nullptr;
					}
				}
			}
			return nullptr;
		}
		
		::MysoreScript::Closure* defineFunction(::MysoreScript::Interpreter::Context& context, std::string name, size_t arguments) {
			::MysoreScript::Closure* closure = gcAlloc<::MysoreScript::Closure>(0);
			closure->isa = &::MysoreScript::ClosureClass;
			closure->parameters = ::MysoreScript::createSmallInteger(arguments);
			context.setSymbol(name, reinterpret_cast<::MysoreScript::Obj>(closure));
			return closure;
		}
		
		MysoreScript::MysoreScript(Interpreter::Context& epimyContext) {
			defineFunction(context, "unify", 2)->invoke = reinterpret_cast<::MysoreScript::ClosureInvoke>(unify);
		}

		void MysoreScript::execute(std::string string) {
			std::unique_ptr<::MysoreScript::AST::Statements> root;
			pegmatite::StringInput input(string);
			if (parser.parse(input, parser.g.statements, parser.g.ignored, errorReporter, root)) {
				root->interpret(context);
				previousASTs.push_back(std::move(root));
			} else {
				throw ::Epilog::CompilationException("Could not parse the MysoreScript block.", __FILENAME__, __func__, __LINE__);
			}
		}
	}
}
