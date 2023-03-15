#ifndef INODE_HPP_
#define INODE_HPP_

#include <vector>
#include <stdint.h>

#include "backend.hpp"
#include "const.hpp"
#include "dcfs.hpp"

class RecordCache {
public:
	RecordCache(std::string file_hashname, 
		uint64_t block_size, 
		uint64_t blockmap_cover, 
		//std::vector<std::string> *blockmap_vec, 
		std::string *blockmap_hashname,
		uint64_t *i_size,
		StorageBackend *backend):
		hashname_(file_hashname),
		block_size_(block_size),
		blockmap_cover_(blockmap_cover),
		//dblock_num_(0),
		dcache_map_(),
		//bmblock_num_(0),
		//bmcache_stats_(),
		//bmcache_blocks_(),
		//bmcache_map_(),
		//pblockmap_vec_(blockmap_vec),
		blockmap_(NULL),	
		pblockmap_hasname_(blockmap_hashname),
		psize_(i_size),
		dcache_stats_(*i_size / block_size + 1),
		dcache_blocks_(*i_size / block_size + 1),
		backend_(backend) { 
			assert(block_size_ > 0); 
		}

	~RecordCache() {
		for(uint64_t i = 0; i < dcache_blocks_.size(); i++)
			if (dcache_blocks_[i])
				delete [] dcache_blocks_[i];
		
		delete [] blockmap_;
		//for(uint64_t i = 0; i < bmcache_blocks_.size(); i++)
		//	if (bmcache_blocks_[i])
		//		delete [] bmcache_blocks_[i];
	}

	int read(void *buf, uint64_t offset, uint64_t size);
	int write(const void *buf, uint64_t offset, uint64_t size);


private:
	struct cstat {
		cstat(): cached(false), dirty(false){}
		bool cached;
		bool dirty;
	};

	int fill_dcache(uint64_t offset, uint64_t size);
	int fill_bmcache(uint64_t block_offset, uint64_t cnt);
	bool blockmap_check(uint64_t blk_idx, std::string *hash);

	const std::string hashname_;
	const uint64_t block_size_; // in bytes
	//uint64_t dblock_num_;
	std::vector<cstat> dcache_stats_;
	std::vector<char*> dcache_blocks_;
	std::map<uint64_t, std::string>	dcache_map_;

	const uint64_t blockmap_cover_; // in block cnt
	char* blockmap_;
	//uint64_t bmblock_num_;
	//std::vector<cstat> bmcache_stats_;
	//std::vector<char*> bmcache_blocks_;
	//std::map<uint64_t, std::vector<std::string>>	bmcache_map_;

	//std::vector<std::string> *pblockmap_vec_;
	std::string *pblockmap_hasname_;

	uint64_t *psize_;
	StorageBackend *backend_;
};


/**
 * In-memory handle of a file (storage format = a single DC)
 * Inode object represents a single version of file (= single InodeRecord).
 * When reference count becomes zero, the inode object is released, and the cache is flushed.
 * Assuming single-level block map (1/512 overhead) for 1st iteration
*/
class Inode {
public:
	Inode(std::string file_hashname, 
		std::string ino_hashname,
		std::string blockmap_hashname, 
		uint64_t block_size_in_kb, 
		uint64_t blockmap_cover,
		std::vector<std::string> blockmap_vec,
		uint64_t i_size, 
		StorageBackend *backend): 
		i_file_hashname_(file_hashname), 
		i_ino_hashname_(ino_hashname),
		i_size_(i_size), 
		i_block_size_(block_size_in_kb * 1024), 
		i_blockmap_hashname_(blockmap_hashname),
		i_blockmap_cover_(blockmap_cover),
		//i_blockmap_vec_(blockmap_vec),
		i_backend_(backend),
		i_cache_(new RecordCache(file_hashname, 
				i_block_size_, 
				&i_blockmap_hashname_,
				i_blockmap_cover_,
				&i_size_, 
				//&i_blockmap_vec_, 
				backend)),
		i_ref_count_(0) {}

	~Inode() { delete i_cache_; }

	int read(void *buf, uint64_t offset, uint64_t size);
	int write(const void *buf, uint64_t offset, uint64_t size);
	uint64_t size() const;
	std::string hashname() const;
	void ref();
	void unref();

private:
	std::string i_file_hashname_;
	std::string i_ino_hashname_;
	uint64_t i_size_;

	uint64_t i_block_size_;
	uint64_t i_blockmap_cover_; // # of data blocks a single block map covers
	//std::vector<std::string> i_blockmap_vec_;
	std::string i_blockmap_hashname_;

	StorageBackend *i_backend_;
	RecordCache *i_cache_;

	uint64_t i_ref_count_;
};


Inode *allocate_inode(DCFS *dcfs);
Inode *get_inode(DCFS *dcfs, std::string file_hashname);
void init_inode();

#endif