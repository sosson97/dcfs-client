#ifndef DIR_HPP_
#define DIR_HPP_

#include <vector>
#include <string>
#include <mutex>

#include <stdint.h>

#include "errno.hpp"

class DirectoryEntry {
public:
	DirectoryEntry(std::string filename, std::string hashname): d_filename_(filename), d_hashname_(hashname) {}
	std::string Filename();
	std::string Hashname();

private:
	std::string d_filename_;
	std::string	d_hashname_;
};


class Directory {
public:
	Directory(): it_(entries_.begin()) {}
	void InitReaddir();
	DirectoryEntry *Readdir();
	err_t Addent(DirectoryEntry *newent);
private:
	std::vector<DirectoryEntry*>::iterator it_;
	std::vector<DirectoryEntry*> entries_;
	std::mutex m_;
};
#endif