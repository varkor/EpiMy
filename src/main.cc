#include <iostream>

void usage(const char command[]) {
	std::cerr << "usage: " << command << " <file>" << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
		
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
