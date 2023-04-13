#include <vector>

#include <cstring>
#include <cassert>

#include "inode.hpp"
#include "dcfs.hpp"

#include "util/logging.hpp"
// index to in-memory Inode
static std::map<std::string, Inode *> inode_table;

static std::mutex inode_table_mutex;

static char zero_str[HASHLEN_IN_BYTES];

void init_inode() {
	memset(zero_str, 0, HASHLEN_IN_BYTES);
}

/**
 * Inefficient but correct locking; I locked the entire alloc/get inode function to avoid "double requested while reading from backend" case.
 * This always happens when we store the result of IO operations in memory ...
*/

Inode *allocate_inode(DCFS *dcfs) {
	std::lock_guard<std::mutex> lock(inode_table_mutex);
	std::string hashname;

	err_t err =	dcfs->backend->CreateNewFile(&hashname);
	if (err < 0) {
		Logger::log(ERROR, "Failed to allocate new inode, err: " + err);
		return NULL;
	}

	Inode *ret = new Inode(hashname, dcfs->block_size_in_kb, BLOCKMAP_COVER, dcfs->backend);
	inode_table[hashname] = ret;

	return ret;
}

Inode *get_inode(DCFS *dcfs, std::string hashname) {
	std::lock_guard<std::mutex> lock(inode_table_mutex);
	buf_desc_t desc;

	auto match = inode_table.find(hashname);	// find cached entry
	if (match != inode_table.end())
		return match->second;


	// not in index, ask backend for meta
	alloc_buf_desc(&desc, MAX_FILEMETA_SIZE);
	uint64_t read_size;
	std::string recordname;
	err_t err = dcfs->backend->ReadFileMeta(hashname, &recordname, &desc, &read_size);
	if (err < 0)
		return NULL;

	// Deserialize meta
	uint64_t index_to_data, file_size_in_bytes;
	memcpy(&index_to_data, desc.buf + sizeof(uint64_t), sizeof(uint64_t));
	memcpy(&file_size_in_bytes, desc.buf + index_to_data, sizeof(uint64_t));
	std::string blockmap_hash = std::string(desc.buf + index_to_data + sizeof(uint64_t), HASHLEN_IN_BYTES);

	Inode *ret = new Inode(hashname,
							recordname,
							blockmap_hash,
							dcfs->block_size_in_kb, 
							BLOCKMAP_COVER, 
							file_size_in_bytes, 
							dcfs->backend);	
	inode_table[hashname] = ret;
	dealloc_buf_desc(&desc);

	return ret;
}

Inode::Inode(std::string hashname, 
	std::string ino_recordname,
	std::string bm_recordname, 
	uint64_t block_size_in_kb, 
	uint64_t bm_cover,
	uint64_t i_size, 
	StorageBackend *backend):		 
		i_hashname_(hashname), 
		i_ino_recordname_(ino_recordname),
		i_size_(i_size), 
		i_block_size_(block_size_in_kb * 1024), 
		i_bm_cover_(bm_cover),
		i_bm_recordname_(bm_recordname),
		i_backend_(backend),
		i_cache_(new RecordCache(this)),
		i_ref_count_(0) {}

// fresh Inode constructor
Inode::Inode(std::string hashname, 
	uint64_t block_size_in_kb, 
	uint64_t bm_cover, 
	StorageBackend *backend):
		i_hashname_(hashname),
		i_ino_recordname_(""),
		i_size_(0),
		i_block_size_(block_size_in_kb * 1024),
		i_bm_cover_(bm_cover),
		i_bm_recordname_(""),
		i_backend_(backend),
		i_cache_(new RecordCache(this)),	
		i_ref_count_(0) {}

Inode::~Inode() { delete i_cache_; }


uint64_t Inode::Size() const {
	return i_size_;
}

uint64_t Inode::BlockSize() const {
	return i_block_size_;
}

std::string Inode::Hashname() const {
	return i_hashname_;
}

StorageBackend *Inode::Backend() const {
	return i_backend_;
}

uint64_t Inode::BlockMapCover() const {
	return i_bm_cover_;
}

std::string Inode::BlockMapRecordname() const {
	return i_bm_recordname_;
}

void Inode::Ref() {
	i_ref_count_++;
}

void Inode::Unref() {
	assert(i_ref_count_ > 0);

	i_ref_count_--;

	if (i_ref_count_ == 0) 
		i_cache_->FlushCache();
}

// (offset, size) is guaranteed to be within the file boundary.
err_t RecordCache::Read(void *buf, uint64_t offset, uint64_t size) {
	err_t ret = NO_ERR;
	uint64_t block_size = host_->BlockSize();

	assert(size > 0);	
	
	uint64_t st_block = offset / block_size;
	uint64_t ed_block = (offset + size) / block_size;
	uint64_t st_offset = offset % block_size;
	uint64_t ed_offset = (offset + size) % block_size;

	ret = fillDcache(offset, size);
	if (ret < 0)
		return ret;
		
	/**
	 * optimized for short request?
	*/
	if (st_block == ed_block) {
		memcpy((char *)buf, dcache_blocks_[st_block] + st_offset, size);
	} else if (st_block + 1 == ed_block) {
		memcpy((char *)buf, dcache_blocks_[st_block] + st_offset, block_size - st_offset);
		memcpy((char *)buf + block_size - st_offset, dcache_blocks_[ed_block], ed_offset + 1);
	} else {
		uint64_t cur = block_size - st_offset;
		memcpy((char *)buf, dcache_blocks_[st_block] + st_offset, block_size - st_offset);		

		for (uint64_t blk_idx = st_block + 1; blk_idx < ed_block; blk_idx++) {
			memcpy((char *)buf + cur, dcache_blocks_[blk_idx], block_size);
			cur += block_size;
		}

		memcpy((char *)buf + cur, dcache_blocks_[ed_block], ed_offset + 1);		
	}

	return ret;	
}

/* write-back */
err_t RecordCache::Write(const void *buf, uint64_t offset, uint64_t size) {
	err_t ret = NO_ERR;
	uint64_t block_size = host_->BlockSize();

	assert(size > 0);	

	uint64_t st_block = offset / block_size;
	uint64_t ed_block = (offset + size) / block_size;
	uint64_t st_offset = offset % block_size;
	uint64_t ed_offset = (offset + size) % block_size;

	ret = fillDcache(offset, size);

	if (st_block == ed_block) {
		memcpy(dcache_blocks_[st_block] + st_offset, (char*) buf, size);
		dcache_stats_[st_block].dirty = true;
	} else if (st_block + 1 == ed_block) {
		memcpy(dcache_blocks_[st_block] + st_offset, (char*) buf, block_size - st_offset);
		memcpy(dcache_blocks_[ed_block], (char*) buf + block_size - st_offset, ed_offset + 1);
		dcache_stats_[st_block].dirty = true;
		dcache_stats_[ed_block].dirty = true;
	} else {
		uint64_t cur = block_size - st_offset;
		memcpy(dcache_blocks_[st_block] + st_offset, (char*) buf, block_size - st_offset);	
		dcache_stats_[st_block].dirty = true;
		
		for (uint64_t blk_idx = st_block + 1; blk_idx < ed_block; blk_idx++) {
			memcpy(dcache_blocks_[blk_idx], (char*) buf + cur, block_size);
			dcache_stats_[blk_idx].dirty = true;
			cur += block_size;
		}

		memcpy(dcache_blocks_[ed_block], (char*) buf + cur, ed_offset + 1);		
		dcache_stats_[ed_block].dirty = true;
	}

	return ret;	
}


/* no readahead */
err_t RecordCache::fillDcache(uint64_t offset, uint64_t size) {
	err_t ret = NO_ERR;
	uint64_t block_size = host_->BlockSize();

	uint64_t st_block = offset / block_size;
	uint64_t ed_block = (offset + size) / block_size;
	uint64_t blk_idx;
	std::string recordname;

	if (st_block > dcache_stats_.size()) 
		st_block = dcache_stats_.size(); 

	ret = fillBmcache(st_block, ed_block - st_block + 1);
	if (ret < 0)
		return ret;

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
			dcache_blocks_[blk_idx] = new char[block_size];
		
			if (bmCheck(blk_idx, &recordname)) {
				buf_desc_t desc;
				desc.buf = dcache_blocks_[blk_idx];
				desc.size = block_size;

				uint64_t read_size;
				ret = host_->Backend()->ReadRecordData(host_->Hashname(),
										recordname, 
										&desc,
										&read_size); 
				if (ret < 0)
					return ret;
			}

			dcache_stats_[blk_idx].cached = true;
		}
	}

	return ret;
}

err_t RecordCache::fillBmcache(uint64_t blk_offset, uint64_t cnt) {
	err_t ret = NO_ERR;
	uint64_t bm_cover = host_->BlockMapCover();

	if (!bm_ && host_->BlockMapRecordname().length() > 0) {	
		bm_ = new char[HASHLEN_IN_BYTES * bm_cover];
		memset(bm_, 0, HASHLEN_IN_BYTES * bm_cover);

		buf_desc_t desc;
		desc.buf = bm_;
		desc.size = HASHLEN_IN_BYTES * bm_cover;

		uint64_t read_size;
		ret = host_->Backend()->ReadRecordData(host_->Hashname(), 
						host_->BlockMapRecordname(), 
						&desc, 
						&read_size);
		if (ret < 0)
			return ret;
	}

	return ret;
}


bool RecordCache::bmCheck(uint64_t blk_idx, std::string *recordname) {
	if (!bm_)
		return false;
	uint64_t bm_cover = host_->BlockMapCover();
	char ret[HASHLEN_IN_BYTES];
	
	if (blk_idx >= bm_cover)
		return false;

	memcpy(ret, bm_ + blk_idx * HASHLEN_IN_BYTES, HASHLEN_IN_BYTES);

	if (strcmp(ret, zero_str) == 0) {
		return false;
	} else {
		*recordname = std::string(ret, HASHLEN_IN_BYTES);
		return true;
	}
}


err_t RecordCache::FlushCache() {
	err_t ret = NO_ERR;
	uint64_t block_size = host_->BlockSize();
	std::string hashname = host_->Hashname();

	std::vector<buf_desc_t> desc_vec;
	/**
	 * Possible Opt: merge adjacent dirty blocks and write them in one shot
	 * But this technique needs gathering dirty blocks into a single buffer.
	 * The gathering may not be necessary if we can throw async write requests to TCP stack
	 * since TCP stack uses its own buffer to optimize this case.
	*/
	for (uint64_t blk_idx = 0; blk_idx < dcache_stats_.size(); blk_idx++) {
		if (dcache_stats_[blk_idx].dirty) {
			buf_desc_t desc;
			desc.buf = dcache_blocks_[blk_idx];
			desc.size = block_size;
			desc.file_offset = blk_idx * block_size;
			desc_vec.push_back(desc);
		}
	}

	ret = host_->Backend()->WriteRecord(host_->Hashname(), &desc_vec);
	if (ret < 0)
		return ret;

	return ret;
}

err_t Inode::Read(void *buf, uint64_t offset, uint64_t size, uint64_t *read_size) {
	if (offset >= i_size_)
		return 0;
	
	if (offset + size > i_size_)
		size = i_size_ - offset;

	*read_size = size;
	return i_cache_->Read(buf, offset, size);
}

err_t Inode::Write(const void *buf, uint64_t offset, uint64_t size, uint64_t *write_size) {
	if (offset >= i_size_)
		offset = i_size_;

	int ret = i_cache_->Write(buf, offset, size);

	if (ret == NO_ERR && offset + size > i_size_)
		i_size_ = offset + size;

	*write_size = size;
	return ret;
}
