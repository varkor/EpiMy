#include "ast.hh"

namespace MysoreScript {
	namespace AST {
		Obj LanguageExpression::evaluateExpr(Interpreter::Context& context) {
			if (::EpiMy::Adaptors::currentContext != nullptr) {
				std::shared_ptr<void> result(::EpiMy::Interpreter::evaluateExtract(*::EpiMy::Adaptors::currentContext, language, extract, "MysoreScript"));
				Obj* value = std::static_pointer_cast<Obj>(result).get();
				Obj obj = value ? *value : nullptr;
				return obj;
			} else {
				throw ::Epilog::RuntimeException("Tried to evaluate a language expression with no context.", __FILENAME__, __func__, __LINE__);
			}
		}
	}
}

namespace Epilog {
	namespace AST {
		std::list<Instruction*> LanguageExpression::instructions(std::shared_ptr<TermNode> node, std::unordered_map<std::string, HeapReference>& allocations, bool bodyTerm, bool argumentTerm) const {
			if (::EpiMy::Adaptors::currentContext != nullptr) {
				std::shared_ptr<void> result(::EpiMy::Interpreter::evaluateExtract(*::EpiMy::Adaptors::currentContext, language, extract, "Epilog"));
				HeapReference* value = std::static_pointer_cast<HeapReference>(result).get();
				if (value == nullptr) {
					throw ::Epilog::RuntimeException("Language expression resolved to a null pointer.", __FILENAME__, __func__, __LINE__);
				}
				std::list<Instruction*> instructions;
				if (!bodyTerm) {
					// Head term.
					if (argumentTerm) {
						instructions.push_back(new CopyArgumentToRegisterInstruction(allocations[node->symbol], node->reg));
					} else {
						instructions.push_back(new UnifyVariableInstruction(node->reg));
					}
				} else {
					// Body term.
					if (argumentTerm) {
						instructions.push_back(new PushVariableToAllInstruction(allocations[node->symbol], node->reg));
					} else {
						instructions.push_back(new PushVariableInstruction(node->reg));
					}
				}
				instructions.push_back(new UnifyRegisterAndArgumentInstruction(node->reg, *value));
				return instructions;
			} else {
				throw ::Epilog::RuntimeException("Tried to evaluate a language expression with no context.", __FILENAME__, __func__, __LINE__);
			}
		}
	}
}
