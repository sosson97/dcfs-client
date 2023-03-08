#ifndef BACKEND_HPP_
#define BACKEND_HPP_

#include <vector>
#include <map>

#include <stdint.h>
#include <cassert>
/* local file but xx style*/

#define BACKEND_MNT_POINT "/tmp/backend"

void init_backend();


class StorageBackend {
public:
	StorageBackend(uint64_t inum, uint64_t block_size):
		inum_(inum),
		block_size_(block_size),
		block_num_(0),
		cache_stats_(),
		cache_blocks_(),
		file_map_()
	{ assert(block_size_ > 0); }
	int read(void *buf, uint64_t offset, uint64_t size);
	int write(const void *buf, uint64_t offset, uint64_t size);


private:
	struct cstat {
		cstat(): cached(false), filebacked(false), dirty(false){}
		bool cached;
		bool filebacked;
		bool dirty;
	};

	int fill_cache(uint64_t offset, uint64_t size);

	const uint64_t inum_;
	const uint64_t block_size_; // in bytes
	uint64_t block_num_;
	std::vector<cstat> cache_stats_;
	std::vector<char*> cache_blocks_;
	std::map<uint64_t, std::string>	file_map_;
};

#endif