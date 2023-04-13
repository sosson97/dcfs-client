#include "dir.hpp"

std::string DirectoryEntry::Filename() {
	return d_filename_;
}

std::string DirectoryEntry::Hashname() {
	return d_hashname_;
}

void Directory::InitReaddir() {
	it_ = entries_.begin();
}
DirectoryEntry *Directory::Readdir() {
	DirectoryEntry * ret;
	if (it_ == entries_.end()) {
		it_ = entries_.begin();
		return NULL;
	}
	ret = *it_;
	it_++;
	return ret;
}

err_t Directory::Addent(DirectoryEntry *newent) {
	entries_.push_back(newent);
	it_ = entries_.begin();
	return NO_ERR;	
}