#include "adaptors.hh"

namespace Epilog {
	namespace AST {
		std::pair<Instruction::instructionReference, std::unordered_map<std::string, HeapReference>> generateInstructionsForRule(Interpreter::Context& context, CompoundTerm* head, pegmatite::ASTList<EnrichedCompoundTerm>* goals);
	}
}

namespace EpiMy {
	void epimyErrorReporter(const pegmatite::InputRange& inputRange, const std::string& message) {
		std::cerr << "Syntax error at " << inputRange.start.line << ":" << inputRange.start.col << ": " << inputRange.str() << std::endl;
	}
	pegmatite::ErrorReporter errorReporter = epimyErrorReporter;
	
	namespace Adaptors {
		Interpreter::Context* currentContext;
		
		void initialiseGrammars() {
			auto& epilogGrammar = ::Epilog::Parser::EpilogGrammar::get();
			epilogGrammar.term.expr = epilogGrammar.term.expr | Parser::EpiMyGrammar::get().languageExpression;
			auto& mysoreScriptGrammar = ::MysoreScript::Parser::MysoreScriptGrammar::get();
			mysoreScriptGrammar.expression.expr = mysoreScriptGrammar.expression.expr | Parser::EpiMyGrammar::get().languageExpression;
		}
		
		std::string escapeFunctorName(std::string string) {
			std::string::size_type position = 0;
			while ((position = string.find("'", position)) != std::string::npos) {
				string.replace(position, 1, "\\'");
				position += 2;
			}
			return "'" + string + "'";
		}
		
		::Epilog::HeapReference pushCompoundTerm(::Epilog::HeapFunctor const& functor) {
			::Epilog::HeapReference::heapIndex index = ::Epilog::Runtime::currentRuntime->heap.size();
			::Epilog::HeapTuple header(::Epilog::HeapTuple::Type::compoundTerm, index + 1);
			::Epilog::Runtime::currentRuntime->heap.push_back(header.copy());
			::Epilog::Runtime::currentRuntime->heap.push_back(functor.copy());
			return ::Epilog::HeapReference(::Epilog::StorageArea::heap, index);
		}
		
		void pushParameterReference(::Epilog::HeapReference reference) {
			if (::Epilog::HeapTuple* tuple = dynamic_cast<::Epilog::HeapTuple*>(::Epilog::dereference(reference).getPointer())) {
				// We need to point at the functor rather than the compound term tuple.
				reference.index = tuple->reference;
			}
			::Epilog::Runtime::currentRuntime->heap.push_back(::Epilog::HeapTuple(::Epilog::HeapTuple::Type::compoundTerm, reference.index).copy());
		}
		
		::Epilog::HeapReference convertObjToTerm(::MysoreScript::Obj& obj) {
			// Converts a MysoreScript object into an Epilog term.
			// Currently only integers, strings and arrays are supported.
			if (obj == nullptr) {
				throw ::Epilog::RuntimeException("Tried to convert a null object into a term.", __FILENAME__, __func__, __LINE__);
			}
			if (::MysoreScript::isInteger(obj)) {
				::Epilog::HeapReference::heapIndex index = ::Epilog::Runtime::currentRuntime->heap.size();
				::Epilog::HeapNumber number(::MysoreScript::getInteger(obj));
				::Epilog::Runtime::currentRuntime->heap.push_back(number.copy());
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
			::Epilog::HeapContainer* nextTerm = term;
			bool validList = true;
			while (validList) {
				validList = false;
				if (::Epilog::HeapTuple* tuple = dynamic_cast<::Epilog::HeapTuple*>(nextTerm)) {
					::Epilog::HeapFunctor* functor;
					if (tuple->type == ::Epilog::HeapTuple::Type::compoundTerm && (functor = dynamic_cast<::Epilog::HeapFunctor*>(::Epilog::Runtime::currentRuntime->heap[tuple->reference].get()))) {
						std::string symbol = functor->toString();
						if (symbol == "./2") {
							elements.push_back(convertTermToObj(::Epilog::HeapReference(::Epilog::StorageArea::heap, tuple->reference + 1)));
							nextTerm = ::Epilog::Runtime::currentRuntime->heap[tuple->reference + 2].get();
							validList = true;
						} else if (symbol == "[]/0") {
							return true;
						}
					}
				}
			}
			return false;
		}
		
		::MysoreScript::Obj convertTermToObj(const ::Epilog::HeapReference& reference) {
			// Converts an Epilog term into a MysoreScript object.
			// Epilog lists correspond to MysoreScript arrays, integers correspond to integers, and any other compound term is converted into a string. Unbound variables are converted into null values.
			::Epilog::HeapReference address(::Epilog::dereference(reference));
			::Epilog::HeapContainer* term = address.getPointer();
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
		
		std::shared_ptr<void> Adaptor::convertValue(std::shared_ptr<void> value, const std::string& sourceLanguage, const std::string& targetLanguage) {
			if (sourceLanguage == targetLanguage) {
				return value;
			}
			if (targetLanguage == "Epilog") {
				if (sourceLanguage == "MysoreScript") {
					::MysoreScript::Obj* result = std::static_pointer_cast<::MysoreScript::Obj>(value).get();
					::MysoreScript::Obj obj = result ? *result : nullptr;
					return std::shared_ptr<void>(new ::Epilog::HeapReference(convertObjToTerm(obj)));
				}
			}
			if (targetLanguage == "MysoreScript") {
				if (sourceLanguage == "Epilog") {
					::Epilog::HeapReference* result = std::static_pointer_cast<::Epilog::HeapReference>(value).get();
					return std::shared_ptr<void>(result ? new ::MysoreScript::Obj(convertTermToObj(*result)) : nullptr);
				}
			}
			throw ::Epilog::RuntimeException("Unable to convert a value between the specified languages.", __FILENAME__, __func__, __LINE__);
		}
		
		Epilog::Epilog(Interpreter::Context& epimyContext) {
			::Epilog::Runtime::currentRuntime = &runtime;
			
			// Declare a query in the Epilog instruction set that will be executed whenever unification is invoked from MysoreScript
			::Epilog::Runtime::currentRuntime->labels["<mysorescript unify>"] = ::Epilog::Runtime::currentRuntime->instructions->size();
			pushInstruction(context, new ::Epilog::AllocateInstruction(0));
			pushInstruction(context, new ::Epilog::CallInstruction(::Epilog::HeapFunctor("<temporary unification identifier>", 0)));
			pushInstruction(context, new ::Epilog::DeallocateInstruction());
			
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
						if ((value = adaptor->context.lookupSymbol(::Epilog::Runtime::currentRuntime->registers[0]->trace())) != nullptr) {
							if (::Epilog::UnifyRegisterAndArgumentInstruction* instruction = dynamic_cast<::Epilog::UnifyRegisterAndArgumentInstruction*>((*::Epilog::Runtime::currentRuntime->instructions)[::Epilog::Runtime::currentRuntime->nextInstruction + 1].get())) {
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
						adaptor->context.setSymbol(::Epilog::Runtime::currentRuntime->registers[0]->trace(), convertTermToObj(::Epilog::HeapReference(::Epilog::StorageArea::reg, 1)));
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
						if ((value = adaptor->context.lookupSymbol(::Epilog::Runtime::currentRuntime->registers[0]->trace())) != nullptr) {
							::MysoreScript::Obj obj = *value;
							if (!::MysoreScript::isInteger(*value) && obj->isa == &::MysoreScript::ClosureClass) {
								::MysoreScript::Closure* closure = reinterpret_cast<::MysoreScript::Closure*>(obj);
								::MysoreScript::Obj argumentObj = convertTermToObj(::Epilog::HeapReference(::Epilog::StorageArea::reg, 1));
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
									if (::Epilog::UnifyRegisterAndArgumentInstruction* instruction = dynamic_cast<::Epilog::UnifyRegisterAndArgumentInstruction*>((*::Epilog::Runtime::currentRuntime->instructions)[::Epilog::Runtime::currentRuntime->nextInstruction + 1].get())) {
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
		
		void Epilog::execute(pegmatite::Input& input) {
			std::unique_ptr<::Epilog::AST::Clauses> root;
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
		
		void Epilog::execute(const std::string& string) {
			pegmatite::StringInput i(string);
			Epilog::execute(i);
		}
		
		void Epilog::execute(std::ifstream stream) {
			pegmatite::StreamInput input(pegmatite::StreamInput::Create("Epilog", stream));
			Epilog::execute(input);
		}
		
		void executeInstruction(::Epilog::Instruction::instructionReference unificationQuery) {
			::Epilog::AST::executeInstructions(unificationQuery, unificationQuery + 1, nullptr);
		}
		
		::Epilog::HeapReference::heapIndex copyContainerToRuntime(::Epilog::HeapReference reference, ::Epilog::Runtime* runtime) {
			::Epilog::HeapContainer* container = reference.getPointer();
			::Epilog::HeapReference::heapIndex index = runtime->heap.size();
			if (::Epilog::HeapTuple* tuple = dynamic_cast<::Epilog::HeapTuple*>(container)) {
				if (tuple->type == ::Epilog::HeapTuple::Type::compoundTerm) {
					runtime->heap.push_back(std::unique_ptr<::Epilog::HeapContainer>(new ::Epilog::HeapTuple(::Epilog::HeapTuple::Type::compoundTerm, index + 1)));
					if (::Epilog::HeapFunctor* functor = dynamic_cast<::Epilog::HeapFunctor*>(::Epilog::HeapReference(::Epilog::StorageArea::heap, tuple->reference).getPointer())) {
						for (int64_t i = 0; i <= functor->parameters; ++ i) {
							copyContainerToRuntime(::Epilog::HeapReference(::Epilog::StorageArea::heap, tuple->reference + i), runtime);
						}
					} else {
						throw ::Epilog::RuntimeException("Encountered a compound term not pointing to a functor.", __FILENAME__, __func__, __LINE__);
					}
				} else if (tuple->type == ::Epilog::HeapTuple::Type::reference) {
					if (reference.area == ::Epilog::StorageArea::heap && tuple->reference == reference.index) {
						runtime->heap.push_back(std::unique_ptr<::Epilog::HeapContainer>(new ::Epilog::HeapTuple(::Epilog::HeapTuple::Type::reference, index)));
					} else {
						return copyContainerToRuntime(::Epilog::HeapReference(::Epilog::StorageArea::heap, tuple->reference), runtime);
					}
				}
			} else if (dynamic_cast<::Epilog::HeapFunctor*>(container) || dynamic_cast<::Epilog::HeapNumber*>(container)) {
				runtime->heap.push_back(container->copy());
			} else {
				throw ::Epilog::RuntimeException("Encountered unknown heap container.", __FILENAME__, __func__, __LINE__);
			}
			return index;
		}
		
		std::shared_ptr<void> Epilog::evaluate(const std::string& string) {
			pegmatite::StringInput input("?- =(X, " + string + ")");
			std::unique_ptr<::Epilog::AST::Query> root;
			::Epilog::Runtime* previousRuntime = ::Epilog::Runtime::currentRuntime;
			if (parser.parse(input, parser.grammar.query, parser.grammar.ignored, errorReporter, root)) {
				// There could well be existing Epilog active contexts, so we start up a new runtime to make sure we don't overwrite any of the state.
				::Epilog::Interpreter::Context context;
				::Epilog::Runtime temporaryRuntime;
				::Epilog::Runtime::currentRuntime = &temporaryRuntime;
				::Epilog::initialiseBuiltins(context);
				
				auto pair = ::Epilog::AST::generateInstructionsForRule(context, nullptr, &root->body->goals);
				auto startAddress = pair.first;
				::Epilog::AST::executeInstructions(startAddress, ::Epilog::Runtime::currentRuntime->instructions->size() - 1, nullptr); // Don't execute the deallocate instruction until we've extracted the variable from it.
				::Epilog::HeapReference::heapIndex index = copyContainerToRuntime(::Epilog::HeapReference(::Epilog::StorageArea::environment, 0), previousRuntime);
				::Epilog::AST::executeInstructions(::Epilog::Runtime::currentRuntime->instructions->size() - 1, ::Epilog::Runtime::currentRuntime->instructions->size(), nullptr); // Deallocate.
				while (previousRuntime->registers.size() < temporaryRuntime.registers.size()) {
					previousRuntime->registers.push_back(nullptr);
				}
				
				::Epilog::Runtime::currentRuntime = previousRuntime;
				return std::shared_ptr<void>(new ::Epilog::HeapReference(::Epilog::StorageArea::heap, index));
			} else {
				::Epilog::Runtime::currentRuntime = previousRuntime;
				throw ::Epilog::CompilationException("Could not parse the Epilog term.", __FILENAME__, __func__, __LINE__);
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
				if (dynamic_cast<Epilog*>((*i).get())) {
					// Although we can handle most of the execution process ourselves, in order to actually perform unification, we need to actually push a call instruction to the instruction list as Epilog does not currently support running instructions from a separate instruction vector from the main runtime, and requires an instruction index from which to begin.
					::MysoreScript::String* string = reinterpret_cast<::MysoreScript::String*>(name);
					::MysoreScript::Array* array = reinterpret_cast<::MysoreScript::Array*>(parameters);
					::Epilog::HeapReference::heapIndex length = array->length ? ::MysoreScript::getInteger(array->length) : 0;
					// There could well be existing Epilog active contexts, so we start up a new runtime to make sure we don't overwrite any of the state.
					::Epilog::Runtime* previousRuntime = ::Epilog::Runtime::currentRuntime;
					::Epilog::Runtime temporaryRuntime(*previousRuntime);
					::Epilog::Runtime::currentRuntime = &temporaryRuntime;
					// Make sure we don't overflow the number of Epilog registers.
					while (temporaryRuntime.registers.size() < length) {
						temporaryRuntime.registers.push_back(nullptr);
					}
					// Now, call the corresponding functor.
					if (temporaryRuntime.labels.find("<mysorescript unify>") != temporaryRuntime.labels.end()) {
						::Epilog::Instruction::instructionReference unificationQuery = temporaryRuntime.labels["<mysorescript unify>"];
						if (::Epilog::AllocateInstruction* instruction = dynamic_cast<::Epilog::AllocateInstruction*>((*temporaryRuntime.instructions)[unificationQuery].get())) {
							instruction->variables = length;
						} else {
							throw ::Epilog::RuntimeException("Tried to invoke unification from MysoreScript without an allocate instruction.", __FILENAME__, __func__, __LINE__);
						}
						if (::Epilog::CallInstruction* instruction = dynamic_cast<::Epilog::CallInstruction*>((*temporaryRuntime.instructions)[unificationQuery + 1].get())) {
							instruction->functor.name = string->characters;
							instruction->functor.parameters = length;
						} else {
							throw ::Epilog::RuntimeException("Tried to invoke unification from MysoreScript without a call instruction.", __FILENAME__, __func__, __LINE__);
						}
						// We now want to execute all the instructions associated with the <mysorescript unify> rule we've created.
						try {
							executeInstruction(unificationQuery); // Allocate for the call.
							// Push the arguments to the heap.
							for (::Epilog::HeapReference::heapIndex i = 0; i < length; ++ i) {
								// We can skip the usual put_structure instructions by directly modifying the registers and environment variables. This is because we're in a MysoreScript context, so we're not going to be interfering with any existing instruction execution by doing so, and we're going to be triggering execution of the call instruction immediately following register assignment.
								if (array->buffer[i] != nullptr) {
									::Epilog::HeapReference reference(convertObjToTerm(array->buffer[i]));
									temporaryRuntime.registers[i] = reference.getAsCopy();
									temporaryRuntime.currentEnvironment()->variables[i] = reference.getAsCopy();
								} else {
									// Null objects refer to parameters that we wish to bind to temporary variables, which will be returned if the unification succeeds.
									::Epilog::HeapTuple header(::Epilog::HeapTuple::Type::reference, ::Epilog::Runtime::currentRuntime->heap.size());
									temporaryRuntime.heap.push_back(header.copy());
									temporaryRuntime.registers[i] = header.copy();
									temporaryRuntime.currentEnvironment()->variables[i] = header.copy();
								}
							}
							executeInstruction(unificationQuery + 1); // Call the instruction.
							std::list<::MysoreScript::Obj> bindings;
							for (::Epilog::HeapReference::heapIndex i = 0; i < length; ++ i) {
								bindings.push_back(convertTermToObj(::Epilog::HeapReference(::Epilog::StorageArea::environment, i)));
							}
							executeInstruction(unificationQuery + 2); // Deallocate to clean up after the call.
							::Epilog::Runtime::currentRuntime = previousRuntime;
							return ::MysoreScript::constructArrayObj(bindings);
						} catch (const ::Epilog::UnificationError&) {
							::Epilog::Runtime::currentRuntime = previousRuntime;
							return nullptr;
						}
					} else {
						throw ::Epilog::RuntimeException("Tried to invoke unification from MysoreScript without a predefined unification query.", __FILENAME__, __func__, __LINE__);
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
			// Force MysoreScript to use just the interpreter, for more consistency with the Epilog runtime, which currently only supports interpretation.
			::MysoreScript::Interpreter::executionMethod = ::MysoreScript::Interpreter::ExecutionMethod::forceInterpreter;
			defineFunction(context, "unify", 2)->invoke = reinterpret_cast<::MysoreScript::ClosureInvoke>(unify);
		}
		
		void MysoreScript::execute(pegmatite::Input& input) {
			std::unique_ptr<::MysoreScript::AST::Statements> root;
			if (parser.parse(input, parser.grammar.statements, parser.grammar.ignored, errorReporter, root)) {
				root->interpret(context);
				previousASTs.push_back(std::move(root));
			} else {
				throw ::Epilog::CompilationException("Could not parse the MysoreScript block.", __FILENAME__, __func__, __LINE__);
			}
		}

		void MysoreScript::execute(const std::string& string) {
			pegmatite::StringInput input(string);
			MysoreScript::execute(input);
		}
		
		void MysoreScript::execute(std::ifstream stream) {
			pegmatite::StreamInput input(pegmatite::StreamInput::Create("MysoreScript", stream));
			MysoreScript::execute(input);
		}
		
		std::shared_ptr<void> MysoreScript::evaluate(const std::string& string) {
			pegmatite::StringInput input(string);
			std::unique_ptr<::MysoreScript::AST::Expression> root;
			if (parser.parse(input, parser.grammar.expression, parser.grammar.ignored, errorReporter, root)) {
				::MysoreScript::Obj* value = new ::MysoreScript::Obj(root->evaluate(context));
				previousASTs.push_back(std::move(root));
				return std::shared_ptr<void>(value);
			} else {
				throw ::Epilog::CompilationException("Could not parse the MysoreScript expression.", __FILENAME__, __func__, __LINE__);
			}
		}
		
		EpiMy::EpiMy() { }
		
		void EpiMy::execute(pegmatite::Input& input) {
			std::unique_ptr<::EpiMy::AST::LanguageBlocks> root;
			if (parser.parse(input, parser.grammar.languageBlocks, parser.grammar.ignored, errorReporter, root)) {
				root->interpret(context);
			} else {
				throw ::Epilog::CompilationException("Could not parse the EpiMy block.", __FILENAME__, __func__, __LINE__);
			}
		}
		
		void EpiMy::execute(const std::string& string) {
			pegmatite::StringInput input(string);
			EpiMy::execute(input);
		}
		
		void EpiMy::execute(std::ifstream stream) {
			pegmatite::StreamInput input(pegmatite::StreamInput::Create("EpiMy", stream));
			EpiMy::execute(input);
		}
		
		std::shared_ptr<void> EpiMy::evaluate(const std::string& string) {
			throw ::Epilog::CompilationException("EpiMy subexpressions cannot be evaluated in isolation.", __FILENAME__, __func__, __LINE__);
		}
	}
}
