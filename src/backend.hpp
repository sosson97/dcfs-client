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
using signature_t = std::string;

/**
 * DCFS Middleware interface
 * It provides the implementation of the DCFS middleware protocol
*/

struct buf_desc_t {
	char *buf;
	uint64_t size;
	uint64_t file_offset; // used in WriteRecord
};

void alloc_buf_desc(buf_desc_t *desc, uint64_t size);
void dealloc_buf_desc(buf_desc_t *desc);

class DCFSMid {
public:
	virtual err_t Upload(std::string pub, 
				std::string filename, 
				const buf_desc_t *desc, // in
				signature_t sig);
	virtual err_t Modify(std::string pub, 
				std::string filename,
				const buf_desc_t *desc, // in
				signature_t sig);

				// wait, what I need is metadata read. I'll handle other things!
				// What do I need? --> hashes of blockmap! 
	virtual err_t ReadMeta(std::string hashname,
				buf_desc_t *desc, // out
				signature_t sig);	
};

class DCServer {
public:
	/**
	 * Read a record from DCServer using hashname
	 * will be used by clients/MW
	*/
	virtual err_t ReadRecord(std::string hashname, buf_desc_t *desc);

	/**
	 * Write a record to DCServer using hashname
	 * will be used by MW
	*/
	virtual err_t WriteRecord(std::string hashname, const buf_desc_t *desc);
};


/**
 * Be careful about using hashnames!
 * We have two types of hashnames:
 * 1. hashname of the file, which is the hashname of the file DataCapsule -- noted as hashname 
 * 2. hashname of each record. e.g. InodeRecord, BlockMapRecord, DataBlockRecord -- noted as recordname
*/

class StorageBackend {
public:
	StorageBackend() {}
	/** 
	 * Ask DCFS middleware to give file metadata necessary for later file operations e.g. list of blockmap hashes
	*/
	err_t ReadFileMeta(std::string hashname, buf_desc_t *desc);

	/**
	 * Read a record from DCServer using recordname
	*/
	err_t ReadRecord(std::string recordname, buf_desc_t *desc);

	/**
	 * Ask DCFS middleware to write a contiguous block to the file identified by hashname.
	 * Middleware will handle the details of writing to the correct DataCapsule.
	 * Note for implementation: use async send and return when all the data is written if possible.
	 * 
	*/
	err_t WriteRecord(std::string hashname, std::vector<const buf_desc_t> *desc_vec);

	/**
	 * Ask DCFS middleware to allocate a new file DataCapsule and return hashname of it.
	 * We could have reused Write() but this approach is preferred to manage the lifetime of Inode.
	*/
	err_t CreateNewFile(std::string *hashname);

	/**
	 * Ask DCFS middleware to load the root directory of the file system
	*/
	err_t LoadRoot(Directory *root);


private:
	DCFSMid *middleware_;
	DCServer *dcserver_;
};

#endif