#ifndef INODE_HPP_
#define INODE_HPP_

#include <vector>
#include <stdint.h>

#include "backend.hpp"
/**
 * In-memory handle of a file (storage format = a single DC)
*/
class Inode {
public:
	Inode(uint64_t ino, uint64_t block_size_in_kb): 
		i_ino_(ino), 
		i_size_(0), 
		i_block_size_(block_size_in_kb), 
		backend_(ino, block_size_in_kb * 1024) {}

	int read(void *buf, uint64_t offset, uint64_t size);
	int write(const void *buf, uint64_t offset, uint64_t size);
	uint64_t size() const;
	uint64_t inum() const;
private:
	uint64_t i_ino_;
	uint64_t i_size_;
	uint64_t i_block_size_;
	StorageBackend backend_;
};


Inode *allocate_inode();
Inode *get_inode(uint64_t ino);
void init_inode();

#endif