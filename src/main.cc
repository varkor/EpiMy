#include <bitset>
#include <dirent.h>
#include <fcntl.h>
#include <gc.h>
#include <time.h>
#include <unordered_map>
#include "adaptors.hh"

void usage(const char command[]) {
	std::cerr << "usage: [-b | --benchmark] " << command << " <file>" << std::endl;
}

int executeEpiMy(pegmatite::Input& input) {
	EpiMy::Parser::EpiMyParser parser;
	EpiMy::Interpreter::Context context;
	std::unique_ptr<EpiMy::AST::LanguageBlocks> root;
	if (parser.parse(input, parser.grammar.languageBlocks, parser.grammar.ignored, EpiMy::errorReporter, root)) {
		try {
			root->interpret(context);
			return EXIT_SUCCESS;
		} catch (const ::Epilog::Exception& exception) {
			exception.print();
			return EXIT_FAILURE;
		}
	} else {
		return EXIT_FAILURE;
	}
}

int executeEpiMy(std::ifstream stream) {
	pegmatite::StreamInput input(pegmatite::StreamInput::Create("EpiMy", stream));
	return executeEpiMy(input);
}

std::unordered_map<std::string, std::string> filesInDirectory(const std::string& basePath, const std::string& extension) {
	std::unordered_map<std::string, std::string> paths;
	DIR* directory;
	if ((directory = opendir(basePath.c_str())) != NULL) {
		struct dirent* entry;
		while ((entry = readdir(directory)) != NULL) {
			std::string fullname = entry->d_name;
			std::string::size_type position;
			// Skip hidden files and directories (or simply files with no extension).
			if (strncmp(".", fullname.c_str(), 1) != 0 && (position = fullname.find(".")) != std::string::npos) {
				std::string name = fullname.substr(0, position);
				if (fullname.substr(position + 1) == extension) {
					paths[name] = basePath + "/" + fullname;
				}
			}
		}
	}
	return paths;
}



// Run the tests for Epilog, MysoreScript and EpiMy to make sure the compiler is working correctly.
int test() {
	const std::string basePath("../tests");
	EpiMy::Interpreter::Context context;
	std::unordered_map<std::string, std::string> files;
	int64_t totalTests = 0;
	int64_t totalPassed = 0;
	int64_t tests;
	int64_t passed;
	
	// Epilog tests.
	std::cerr << "Running Epilog tests:" << std::endl;
	tests = 0;
	passed = 0;
	files = filesInDirectory(basePath + "/Epilog", "el");
	for (auto& pair : files) {
		std::string name = pair.first;
		std::string path = pair.second;
		// Execute the Epilog test.
		++ tests;
		bool successful = true;
		EpiMy::Adaptors::Epilog epilogAdaptor(context);
		std::cerr << "\t";
		auto startAddress = Epilog::pushInstruction(epilogAdaptor.context, new ::Epilog::AllocateInstruction(0));
		Epilog::pushInstruction(epilogAdaptor.context, new ::Epilog::CallInstruction(::Epilog::HeapFunctor("test", 0)));
		Epilog::pushInstruction(epilogAdaptor.context, new ::Epilog::DeallocateInstruction());
		try {
			epilogAdaptor.execute(std::ifstream(path));
			if (epilogAdaptor.runtime.labels.find("test/0") != epilogAdaptor.runtime.labels.end()) {
				Epilog::AST::executeInstructions(startAddress, startAddress + 3, nullptr);
			} else {
				successful = false;
				std::cerr << "?";
			}
		} catch (const ::Epilog::Exception& exception) {
			successful = false;
			std::cerr << "✗";
		}
		if (successful) {
			++ passed;
			std::cerr << "✓";
		}
		std::cerr << "\t" << name << std::endl;
	}
	totalTests += tests;
	totalPassed += passed;
	std::cerr << passed << "/" << tests << "\tpassed.\n" << std::endl;
	
	// MysoreScript tests.
	std::cerr << "Running MysoreScript tests:" << std::endl;
	tests = 0;
	passed = 0;
	files = filesInDirectory(basePath + "/MysoreScript", "ms");
	for (auto& pair : files) {
		std::string name = pair.first;
		std::string path = pair.second;
		// Execute the MysoreScript test.
		++ tests;
		bool successful = false;
		EpiMy::Adaptors::MysoreScript mysorescriptAdaptor(context);
		std::cerr << "\t";
		try {
			mysorescriptAdaptor.execute(std::ifstream(path));
			::MysoreScript::Obj* value;
			if ((value = mysorescriptAdaptor.context.lookupSymbol("test")) != nullptr) {
				::MysoreScript::Obj obj = *value;
				if (!::MysoreScript::isInteger(*value) && obj->isa == &::MysoreScript::ClosureClass) {
					::MysoreScript::Closure* closure = reinterpret_cast<::MysoreScript::Closure*>(obj);
					::MysoreScript::currentContext = &mysorescriptAdaptor.context;
					::MysoreScript::Obj returnValue = ::MysoreScript::callCompiledClosure(closure->invoke, closure, nullptr, 0);
					if ((reinterpret_cast<intptr_t>(returnValue)) & ~7) { // Truth in MysoreScript.
						successful = true;
					} else {
						std::cerr << "✗";
					}
				} else {
					std::cerr << "?";
				}
			} else {
				std::cerr << "?";
			}
		} catch (const ::Epilog::Exception& exception) {
			successful = false;
			std::cerr << "✗";
		}
		if (successful) {
			++ passed;
			std::cerr << "✓";
		}
		std::cerr << "\t" << name << std::endl;
	}
	totalTests += tests;
	totalPassed += passed;
	std::cerr << passed << "/" << tests << "\tpassed.\n" << std::endl;
	
	if (totalPassed < totalTests) {
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}

int benchmark() {
	enum Languages { Epilog, MysoreScript, EpiMy };
	const std::string basePath("../benchmarks");
	std::unordered_map<std::string, std::bitset<3>> benchmarks;
	DIR* directory;
	if ((directory = opendir(basePath.c_str())) != NULL) {
		struct dirent* entry;
		while ((entry = readdir(directory)) != NULL) {
			std::string fullname = entry->d_name;
			std::string::size_type position;
			// Skip hidden files and directories.
			if (strncmp(".", fullname.c_str(), 1) != 0 && (position = fullname.find(".")) != std::string::npos) {
				std::string name = fullname.substr(0, position);
				const char* extension = fullname.substr(position + 1).c_str();
				int index = strcmp("el", extension) == 0 ? Languages::Epilog : strcmp("ms", extension) == 0 ? Languages::MysoreScript : strcmp("epimy", extension) == 0 ? Languages::EpiMy : -1;
				if (index != -1) {
					benchmarks[name][index] = true;
				}
			}
		}
		closedir(directory);
	} else {
		return EXIT_FAILURE;
	}
	for (auto& pair : benchmarks) {
		const std::string& name = pair.first;
		const std::bitset<3>& languages = pair.second;
		if (!languages[Languages::Epilog] || !languages[Languages::MysoreScript] || !languages[Languages::EpiMy]) {
			std::cerr << "The benchmark \"" << name << "\" is missing a" + std::string(!languages[Languages::Epilog] ? "n Epilog" : !languages[Languages::MysoreScript] ? " MysoreScript" : "n EpiMy") + " benchmark." << std::endl;
		}
		std::cerr << "Executing " << name << ":" << std::endl;
		
		EpiMy::Interpreter::Context context;
		const std::string path(basePath + "/" + name);
		if (languages[Languages::Epilog]) {
			std::cerr << "\tEpilog:" << std::endl;
			EpiMy::Adaptors::Epilog epilogAdaptor(context);
			clock_t start = clock();
			try {
				epilogAdaptor.execute(std::ifstream(path + ".el"));
			} catch (const ::Epilog::Exception& exception) {
				exception.print();
			}
			clock_t end = clock();
			std::cerr << "\t\t" << ((double) (end - start)) / CLOCKS_PER_SEC * 1000 << " ms" << std::endl;
		}
		if (languages[Languages::MysoreScript]) {
			std::cerr << "\tMysoreScript:" << std::endl;
			EpiMy::Adaptors::MysoreScript mysorescriptAdaptor(context);
			clock_t start = clock();
			try {
				mysorescriptAdaptor.execute(std::ifstream(path + ".ms"));
			} catch (const ::Epilog::Exception& exception) {
				exception.print();
			}
			clock_t end = clock();
			std::cerr << "\t\t" << ((double) (end - start)) / CLOCKS_PER_SEC * 1000 << " ms" << std::endl;
		}
		if (languages[Languages::EpiMy]) {
			std::cerr << "\tEpiMy:" << std::endl;
			clock_t start = clock();
			executeEpiMy(std::ifstream(path + ".epimy"));
			clock_t end = clock();
			std::cerr << "\t\t" << ((double) (end - start)) / CLOCKS_PER_SEC * 1000 << " ms" << std::endl;
		}
	}
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage(argv[0]);
		
		return EXIT_FAILURE;
	} else {
		GC_init();
		
		// Enable language expressions to be used in the different languages.
		EpiMy::Adaptors::initialiseGrammars();
		
		if (strcmp("-t", argv[1]) == 0 || strcmp("--test", argv[1]) == 0) {
			return test();
		} else if (strcmp("-b", argv[1]) == 0 || strcmp("--benchmark", argv[1]) == 0) {
			return benchmark();
		} else {
			return executeEpiMy(std::ifstream(argv[1]));
		}
	}
}
