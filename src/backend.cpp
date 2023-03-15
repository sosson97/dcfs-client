#include <fstream>
#include <cstring>
#include <cassert>
#include "backend.hpp"

void init_backend(StorageBackend *backend) {
	backend = new FileBackend(BACKEND_MNT_POINT);
}

static std::string get_last_line(std::string filename) {
	std::ifstream in(filename);
	std::string line;
	while (std::getline(in, line)) {
	}
	return line;
}

static void append_line(std::string filename, std::string line) {
	FILE *fp = fopen(filename.c_str(), "a");
	if (fp == NULL) {
		return;
	}
	fprintf(fp, "\n%s", line.c_str());
	fclose(fp);
}