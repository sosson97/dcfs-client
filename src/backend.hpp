#ifndef BACKEND_HPP_
#define BACKEND_HPP_

#include <vector>
#include <map>
#include <filesystem>

#include <stdint.h>
#include <cassert>

#include "const.hpp"
#include "dir.hpp"
/* local file but xx style*/

namespace fs = std::filesystem;

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


/* File backend
*  Files are stored in the following format in local file system:
*  [file hashname]-inode
*  [file hashname]-blockmap
*  [file hashname]-datablock
*/

class FileBackend: public StorageBackend {
public:
	FileBackend(std::string mnt_point): mnt_point_(mnt_point) {
		fs::create_directories(BACKEND_MNT_POINT);
	}
	~FileBackend() {}

	int read(std::string hashname, void *buf, uint64_t size);
	int write(const void *buf, uint64_t size);
	int allocate_inode();
	int get_inode(std::string hashname, void *buf, uint64_t size);
	int load_root(Directory *root);

private:
	std::string mnt_point_;
};


#endif