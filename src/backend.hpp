#ifndef BACKEND_HPP_
#define BACKEND_HPP_

#include <vector>
#include <map>

#include <stdint.h>
#include <cassert>

#include "const.hpp"
#include "dir.hpp"
/* local file but xx style*/


void init_backend(StorageBackend *backend);

class StorageBackend {
public:
	StorageBackend() {}
	virtual ~StorageBackend() {}

	virtual int read(std::string hashname, void *buf, uint64_t size) = 0;
	virtual int write(const void *buf, uint64_t size) = 0;
	virtual int allocate_inode() = 0;
	virtual int get_inode(std::string hashname, void *buf, uint64_t size) = 0;
	virtual int load_root(Directory *root) = 0;
};

class FileBackend: public StorageBackend {
public:
	FileBackend() {}
	~FileBackend() {}

	int read(std::string hashname, void *buf, uint64_t size);
	int write(const void *buf, uint64_t size);
	int allocate_inode();
	int get_inode(std::string hashname, void *buf, uint64_t size);
	int load_root(Directory *root);
};


#endif