#ifndef DIR_HPP_
#define DIR_HPP_

#include <vector>
#include <string>
#include <mutex>

#include <stdint.h>

class DirectoryEntry {
public:
	DirectoryEntry(std::string filename, std::string hashname): d_filename_(filename), d_hashname_(hashname) {}
	std::string filename();
	std::string hashname();

private:
	std::string d_filename_;
	std::string	d_hashname_;
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