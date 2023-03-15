#include <vector>

#include <cstring>
#include <cassert>

#include "inode.hpp"
#include "dcfs.hpp"

static std::map<std::string, Inode *> inode_table;
static std::mutex inode_table_mutex;
static uint64_t ino_cnt = 0;

static char zero_str[HASHLEN_IN_BYTES];

void init_inode() {
	memset(zero_str, 0, HASHLEN_IN_BYTES);
}

Inode *allocate_inode() {
	std::lock_guard<std::mutex> lock(inode_table_mutex);
	dcfs->backend->allocate_inode(); // inode argument read
	Inode *ret = new Inode(...);
	
	return ret;
}

Inode *get_inode(DCFS *dcfs, std::string file_hashname) {
	std::lock_guard<std::mutex> lock(inode_table_mutex);
	auto ret = inode_table.find(file_hashname);	// find cached entry
	if (ret != inode_table.end())
		return ret->second;

	// inode argument read
	dcfs->backend->get_inode(file_hashname, buf, ...); // what is semantic of read?	
	Inode *ret = new Inode(file_hashname, ...);	
	inode_table[file_hashname] = ret;
	return ret;

	return NULL;
}

// (offset, size) is guaranteed to be within the file.
int RecordCache::read(void *buf, uint64_t offset, uint64_t size) {
	assert(size > 0);	
	
	uint64_t st_block = offset / block_size_;
	uint64_t ed_block = (offset + size) / block_size_;
	uint64_t st_offset = offset % block_size_;
	uint64_t ed_offset = (offset + size) % block_size_;

	fill_dcache(offset, size);

	/**
	 * optimized for short request?
	*/
	if (st_block == ed_block) {
		memcpy((char *)buf, dcache_blocks_[st_block] + st_offset, size);
	} else if (st_block + 1 == ed_block) {
		memcpy((char *)buf, dcache_blocks_[st_block] + st_offset, block_size_ - st_offset);
		memcpy((char *)buf + block_size_ - st_offset, dcache_blocks_[ed_block], ed_offset + 1);
	} else {
		uint64_t cur = block_size_ - st_offset;
		memcpy((char *)buf, dcache_blocks_[st_block] + st_offset, block_size_ - st_offset);		

		for (uint64_t blk_idx = st_block + 1; blk_idx < ed_block; blk_idx++) {
			memcpy((char *)buf + cur, dcache_blocks_[blk_idx], block_size_);
			cur += block_size_;
		}

		memcpy((char *)buf + cur, dcache_blocks_[ed_block], ed_offset + 1);		
	}

	return size;	
}

/* write-back */
int RecordCache::write(const void *buf, uint64_t offset, uint64_t size) {
	assert(size > 0);	

	uint64_t st_block = offset / block_size_;
	uint64_t ed_block = (offset + size) / block_size_;
	uint64_t st_offset = offset % block_size_;
	uint64_t ed_offset = (offset + size) % block_size_;

	fill_dcache(offset, size);

	if (st_block == ed_block) {
		memcpy(dcache_blocks_[st_block] + st_offset, (char*) buf, size);
		dcache_stats_[st_block].dirty = true;
	} else if (st_block + 1 == ed_block) {
		memcpy(dcache_blocks_[st_block] + st_offset, (char*) buf, block_size_ - st_offset);
		memcpy(dcache_blocks_[ed_block], (char*) buf + block_size_ - st_offset, ed_offset + 1);
		dcache_stats_[st_block].dirty = true;
		dcache_stats_[ed_block].dirty = true;
	} else {
		uint64_t cur = block_size_ - st_offset;
		memcpy(dcache_blocks_[st_block] + st_offset, (char*) buf, block_size_ - st_offset);	
		dcache_stats_[st_block].dirty = true;
		
		for (uint64_t blk_idx = st_block + 1; blk_idx < ed_block; blk_idx++) {
			memcpy(dcache_blocks_[blk_idx], (char*) buf + cur, block_size_);
			dcache_stats_[blk_idx].dirty = true;
			cur += block_size_;
		}

		memcpy(dcache_blocks_[ed_block], (char*) buf + cur, ed_offset + 1);		
		dcache_stats_[ed_block].dirty = true;
	}

	return size;	
}



/* no readahead */
int RecordCache::fill_dcache(uint64_t offset, uint64_t size) {
	uint64_t st_block = offset / block_size_;
	uint64_t ed_block = (offset + size) / block_size_;
	uint64_t blk_idx;
	std::string *hash;

	if (st_block > dcache_stats_.size()) 
		st_block = dcache_stats_.size(); 

	fill_bmcache(st_block, ed_block - st_block + 1);

	for (blk_idx = st_block; blk_idx <= ed_block; blk_idx++) {
		/**
		 * allocate new cache block if needed
		 * block_num_ is not incrmented
		 * block_num_ denotes # of file-backed blocks
		*/
		if (blk_idx >= dcache_stats_.size()) { 
			dcache_stats_.push_back(cstat());
			dcache_blocks_.push_back(NULL);
		}

		if (!dcache_stats_[blk_idx].cached) {
			dcache_blocks_[blk_idx] = new char[block_size_];
		
			if (blockmap_check(blk_idx, hash)) {
				backend_->read(*hash, dcache_blocks_[blk_idx], block_size_);
			}
			dcache_stats_[blk_idx].cached = true;
		}
	}

	return 0;
}

int RecordCache::fill_bmcache(uint64_t blk_offset, uint64_t cnt) {
	if (!blockmap_) {
		blockmap_ = new char[block_size_ * blockmap_cover_];
		memset(blockmap_, 0, block_size_ * blockmap_cover_);
		backend_->read(*pblockmap_hasname_, blockmap_, block_size_ * blockmap_cover_);
	}
	return 0;
}


bool RecordCache::blockmap_check(uint64_t blk_idx, std::string *hash) {
	assert(blockmap_);
	char ret[HASHLEN_IN_BYTES];
	
	if (blk_idx >= blockmap_cover_)
		return false;

	memcpy(ret, blockmap_ + blk_idx * HASHLEN_IN_BYTES, HASHLEN_IN_BYTES);

	if (strcmp(ret, zero_str) == 0) {
		return false;
	} else {
		*hash = std::string(ret, HASHLEN_IN_BYTES);
		return true;
	}
}


int Inode::read(void *buf, uint64_t offset, uint64_t size) {
	if (offset >= i_size_)
		return 0;
	
	if (offset + size > i_size_)
		size = i_size_ - offset;

	return i_cache_->read(buf, offset, size);
}
int Inode::write(const void *buf, uint64_t offset, uint64_t size) {
	if (offset >= i_size_)
		offset = i_size_;

	int ret = i_cache_->write(buf, offset, size);

	if (!ret && offset + size > i_size_)
		i_size_ = offset + size;	
	return ret;
}


uint64_t Inode::size() const {
	return i_size_;
}

std::string Inode::hashname() const {
	return i_file_hashname_;
}