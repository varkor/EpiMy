#include <bitset>
#include <dirent.h>
#include <fcntl.h>
#include <gc.h>
#include <time.h>
#include <unordered_map>
#include "adaptors.hh"
#include "parser.hh"

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
		if (strcmp("-b", argv[1]) == 0 || strcmp("--benchmark", argv[1]) == 0) {
			return benchmark();
		} else {
			return executeEpiMy(std::ifstream(argv[1]));
		}
	}
}
