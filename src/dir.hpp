#ifndef DIR_HPP_
#define DIR_HPP_

#include <vector>
#include <string>
#include <mutex>

#include <stdint.h>

class DirectoryEntry {
public:
	DirectoryEntry(uint64_t ino, std::string filename): d_ino_(ino), d_filename_(filename) {}
	std::string name();
	uint64_t ino();

private:
	uint64_t	d_ino_;
	std::string d_filename_;
};


class Directory {
public:
	Directory(): it_(entries_.begin()) {}
	void init_readdir();
	DirectoryEntry *readdir();
	int addent(DirectoryEntry *newent);
private:
	std::vector<DirectoryEntry*>::iterator it_;
	std::vector<DirectoryEntry*> entries_;
	std::mutex m_;
};
#endif