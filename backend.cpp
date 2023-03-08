
#include <filesystem>

#include <cstring>
#include <cassert>
#include "backend.hpp"

namespace fs = std::filesystem;

void init_backend() {
	fs::create_directories(BACKEND_MNT_POINT);
}

static void copy_from_file(void *dst, std::string filename, uint64_t size) {
	return;
}

static void create_file(void *src, std::string filename, uint64_t size) {
	return;
}


int StorageBackend::read(void *buf, uint64_t offset, uint64_t size) {
	assert(size > 0);	
	
	uint64_t st_block = offset / block_size_;
	uint64_t ed_block = (offset + size) / block_size_;
	uint64_t st_offset = offset % block_size_;
	uint64_t ed_offset = (offset + size) % block_size_;

	fill_cache(offset, size);

	/**
	 * optimized for short request?
	*/
	if (st_block == ed_block) {
		memcpy((char *)buf, cache_blocks_[st_block] + st_offset, size);
	} else if (st_block + 1 == ed_block) {
		memcpy((char *)buf, cache_blocks_[st_block] + st_offset, block_size_ - st_offset);
		memcpy((char *)buf + block_size_ - st_offset, cache_blocks_[ed_block], ed_offset + 1);
	} else {
		uint64_t cur = block_size_ - st_offset;
		memcpy((char *)buf, cache_blocks_[st_block] + st_offset, block_size_ - st_offset);		

		for (uint64_t blk_idx = st_block + 1; blk_idx < ed_block; blk_idx++) {
			memcpy((char *)buf + cur, cache_blocks_[blk_idx], block_size_);
			cur += block_size_;
		}

		memcpy((char *)buf + cur, cache_blocks_[ed_block], ed_offset + 1);		
	}

	return 0;	
}

/* write-back */
int StorageBackend::write(const void *buf, uint64_t offset, uint64_t size) {
	assert(size > 0);	

	uint64_t st_block = offset / block_size_;
	uint64_t ed_block = (offset + size) / block_size_;
	uint64_t st_offset = offset % block_size_;
	uint64_t ed_offset = (offset + size) % block_size_;

	fill_cache(offset, size);

	if (st_block == ed_block) {
		memcpy(cache_blocks_[st_block] + st_offset, (char*) buf, size);
		cache_stats_[st_block].dirty = true;
	} else if (st_block + 1 == ed_block) {
		memcpy(cache_blocks_[st_block] + st_offset, (char*) buf, block_size_ - st_offset);
		memcpy(cache_blocks_[ed_block], (char*) buf + block_size_ - st_offset, ed_offset + 1);
		cache_stats_[st_block].dirty = true;
		cache_stats_[ed_block].dirty = true;
	} else {
		uint64_t cur = block_size_ - st_offset;
		memcpy(cache_blocks_[st_block] + st_offset, (char*) buf, block_size_ - st_offset);	
		cache_stats_[st_block].dirty = true;
		
		for (uint64_t blk_idx = st_block + 1; blk_idx < ed_block; blk_idx++) {
			memcpy(cache_blocks_[blk_idx], (char*) buf + cur, block_size_);
			cache_stats_[blk_idx].dirty = true;
			cur += block_size_;
		}

		memcpy(cache_blocks_[ed_block], (char*) buf + cur, ed_offset + 1);		
		cache_stats_[ed_block].dirty = true;
	}

	return 0;	
}



/* no readahead */
int StorageBackend::fill_cache(uint64_t offset, uint64_t size) {
	uint64_t st_block = offset / block_size_;
	uint64_t ed_block = (offset + size) / block_size_;
	uint64_t blk_idx;

	if (st_block > cache_stats_.size()) 
		st_block = cache_stats_.size(); 


	for (blk_idx = st_block; blk_idx <= ed_block; blk_idx++) {
		/**
		 * allocate new cache block if needed
		 * block_num_ is not incrmented
		 * block_num_ denotes # of file-backed blocks
		*/
		if (blk_idx >= cache_stats_.size()) { 
			cache_stats_.push_back(cstat());
			cache_blocks_.push_back(NULL);
		}

		if (!cache_stats_[blk_idx].cached) {
			cache_blocks_[blk_idx] = new char[block_size_];
		}
		
		if (cache_stats_[blk_idx].filebacked) {
			copy_from_file(cache_blocks_[blk_idx], std::string(""), block_size_);	
		}
		cache_stats_[blk_idx].cached = true;
	}

	return 0;
}