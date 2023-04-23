#ifndef INODE_HPP_
#define INODE_HPP_

#include <vector>
#include <stdint.h>

#include "backend.hpp"
#include "const.hpp"
#include "dcfs.hpp"

#include "errno.hpp"
#include "util/crypto.hpp"
class RecordCache;

/**
 * In-memory handle of a file (storage format = a single DC)
 * Inode object represents a single version of file (= single InodeRecord).
 * When reference count becomes zero, the inode object is released, and the cache is flushed.
 * ***Assuming single-level block map (1/512 overhead) for 1st iteration
*/
class Inode {
public:
	// load from InodeRecord
	Inode(std::string hashname, 
		std::string ino_recordname,
		std::string bm_recordname, 
		uint64_t block_size_in_kb, 
		uint64_t bm_cover,
		uint64_t i_size,
		std::string aes_key, 
		StorageBackend *backend);

	// fresh Inode constructor
	Inode(std::string hashname, 
		uint64_t block_size_in_kb, 
		uint64_t bm_cover, 
		std::string aes_key,
		StorageBackend *backend);

	~Inode();
	err_t Read(void *buf, uint64_t offset, uint64_t size, uint64_t *read_size);
	err_t Write(const void *buf, uint64_t offset, uint64_t size, uint64_t *write_size);
	uint64_t Size() const;
	uint64_t BlockSize() const;
	std::string Hashname() const;
	StorageBackend *Backend() const;
	uint64_t BlockMapCover() const;
	std::string BlockMapRecordname() const;
	std::string InodeRecordname() const;
	std::string AESKey() const;
	void Ref();
	int Unref();

private:
	std::string i_hashname_; // datacapsule name.
	std::string i_ino_recordname_; // InodeRecord name. An Inode instance is the snapshot of the InodeRecord specified by this field. This field is empty if this inode is not backed by a file yet.
	uint64_t i_size_;

	const uint64_t i_block_size_;
	const uint64_t i_bm_cover_; // # of data blocks a single block map covers
	std::string i_bm_recordname_;

	StorageBackend *i_backend_;
	RecordCache *i_cache_;

	std::string i_aes_key_;

	uint64_t i_ref_count_;
};

class RecordCache {
public:
	RecordCache(Inode *host):
			host_(host),
			dcache_stats_(host_->Size() / host_->BlockSize() + 1),
			dcache_blocks_(host_->Size() / host_->BlockSize() + 1),
			dcache_map_(),
			bm_(NULL)
			{}
	~RecordCache() {
		for(uint64_t i = 0; i < dcache_blocks_.size(); i++)
			if (dcache_blocks_[i])
				delete [] dcache_blocks_[i];
		
		delete [] bm_;
	}

	err_t Read(void *buf, uint64_t offset, uint64_t size);
	err_t Write(const void *buf, uint64_t offset, uint64_t size);

	err_t FlushCache();

private:
	struct cstat {
		cstat(): cached(false), dirty(false){}
		bool cached;
		bool dirty;
	};

	err_t fillDcache(uint64_t offset, uint64_t size);
	err_t fillBmcache(uint64_t block_offset, uint64_t cnt);
	bool bmCheck(uint64_t blk_idx, std::string *recordname);

	Inode *host_;

	std::vector<cstat> dcache_stats_;
	std::vector<char*> dcache_blocks_;
	std::map<uint64_t, std::string>	dcache_map_;

	char* bm_;

};


Inode *allocate_inode(DCFS *dcfs);
Inode *get_inode(DCFS *dcfs, std::string hashname);
void release_inode(DCFS *dcfs, std::string hashname);
void init_inode();

#endif