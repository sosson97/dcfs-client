#include <vector>

#include <cstring>
#include <cassert>

#include "inode.hpp"
#include "dcfs.hpp"

static Inode *inode_table[INO_TABLE_MAX];
static uint64_t ino_cnt = 0;

void init_inode() {
	memset(inode_table, 0, sizeof(Inode *) * INO_TABLE_MAX);
}

Inode *allocate_inode() {
	Inode *ret;
	if (ino_cnt >= INO_TABLE_MAX - 1)
		return NULL;
	++ino_cnt;
	ret = new Inode(ino_cnt, options.block_size_in_kb * 1024);
	inode_table[ino_cnt] = ret;
	return ret;
}

Inode *get_inode(uint64_t ino) {
	Inode *ret = inode_table[ino];	
	return ret;
}

int Inode::read(void *buf, uint64_t offset, uint64_t size) {
	return backend_.read(buf, offset, size);
}
int Inode::write(const void *buf, uint64_t offset, uint64_t size) {
	int ret = backend_.write(buf, offset, size);
	if (!ret && offset + size > i_size_)
		i_size_ = offset + size;	
	return ret;
}


uint64_t Inode::size() const {
	return i_size_;
}

uint64_t Inode::inum() const {
	return i_ino_;
}