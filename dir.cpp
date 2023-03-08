#include "dir.hpp"

std::string DirectoryEntry::name() {
	return d_filename_;
}

uint64_t DirectoryEntry::ino() {
	return d_ino_;
}

void Directory::init_readdir() {
	it_ = entries_.begin();
}
DirectoryEntry *Directory::readdir() {
	DirectoryEntry * ret;
	if (it_ == entries_.end()) {
		it_ = entries_.begin();
		return NULL;
	}
	ret = *it_;
	it_++;
	return ret;
}

int Directory::addent(DirectoryEntry *newent) {
	entries_.push_back(newent);
	it_ = entries_.begin();
	return 0;	
}