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

enum record_type {
	META,
	DIRECTORY,
	INODE,
	BLOCKMAP,
	DATABLOCK,
};

#define SPARE_HASH_SPACE (32 * 10) // 10 hashes
#define RECORD_HEADER_SIZE (32)
#define MAX_INODE_RECORD_SIZE (RECORD_HEADER_SIZE + SPARE_HASH_SPACE + 8 + 32) 
#define MAX_BLOCKMAP_RECORD_SIZE (RECORD_HEADER_SIZE + SPARE_HASH_SPACE + 32 * BLOCKMAP_COVER)


namespace fs = std::filesystem;

using signature_t = std::string;

/**
 * DCFS Middleware interface
 * It provides the implementation of the DCFS middleware protocol
*/
#define MAX_RECORD_SIZE (16 * 1024)

struct buf_desc_t {
	char *buf;
	uint64_t size;
	uint64_t file_offset; // used in WriteRecord
};

void alloc_buf_desc(buf_desc_t *desc, uint64_t size);
void dealloc_buf_desc(buf_desc_t *desc);

class DCFSMid {
public:
	virtual err_t CreateNew(std::string *hashname) = 0;
	virtual err_t Modify(std::string hashname,
				const std::vector<buf_desc_t> *desc_vec) = 0;

	// client expect MW gives the record name of the latest inode record.
	virtual err_t GetInodeName(std::string hashname, 
				std::string *recordname) = 0;

	virtual err_t GetRoot(std::string *hashname, std::string *recordname) = 0;
};

class DCServer {
public:
	/**
	 * Read a record from DCServer using hashname
	 * will be used by clients/MW
	*/
	virtual err_t ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size) = 0;

	/**
	 * Write a record to DCServer using hashname
	 * will be used by MW
	*/
	virtual err_t WriteRecord(std::string dcname, std::string recordname, const buf_desc_t *desc) = 0;
};


class DCFSMidSim : public DCFSMid {
public:
	DCFSMidSim(DCServer *dcserver) : dcserver_(dcserver) {}

	err_t CreateNew(std::string *hashname);
	err_t GetRoot(std::string *hashname, std::string *recordname);
 	err_t GetInodeName(std::string hashname, std::string *recordname);
	err_t Modify(std::string dcname, const std::vector<buf_desc_t> *descs);
private:
	struct InodeRecord {
		InodeRecord() : isize(0), blockmap_hash("") {}

		uint64_t isize;
		std::string blockmap_hash;
	};

	struct BlockMapRecord {
		BlockMapRecord() : hash_to_latest_data_block("") {}

		std::vector<std::string> data_hashes;
		std::string hash_to_latest_data_block;
	};


	err_t composeRecord(record_type type, // in
					std::vector<std::string> *hashes, // in, hashes this record will point
					buf_desc_t *in_desc, // in, data to be written
					buf_desc_t *out_desc, // out, composed record
					std::string *hashname); // out, hash of the record

	DCServer *dcserver_;
	std::map<std::string, std::string> index_; // dcname to latest inode recordname
	std::pair<std::string, std::string> root_; // hashname and recordname of root directory
};


class DCServerSim : public DCServer {
public:
	DCServerSim(std::string mnt_point): mnt_point_(mnt_point) {
		fs::create_directories(BACKEND_MNT_POINT);
	}

	err_t ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size);
	err_t WriteRecord(std::string dcname, std::string recordname, const buf_desc_t *desc);

private:
	std::string mnt_point_;
};


/**
 * Be careful when using hashnames!
 * We have two types of hashnames:
 * 1. hashname of the file, which is the hashname of the file DataCapsule -- noted as hashname 
 * 2. hashname of each record. e.g. InodeRecord, BlockMapRecord, DataBlockRecord -- noted as recordname
*/

class StorageBackend {
public:
	StorageBackend(std::string mnt_point) {
		dcserver_ = new DCServerSim(mnt_point);
		middleware_ = new DCFSMidSim(dcserver_);
	}
	/** 
	 * Ask DCFS middleware to give file metadata necessary for later file operations e.g. list of blockmap hashes
	*/
	err_t ReadFileMeta(std::string hashname, std::string *recordname, buf_desc_t *desc, uint64_t *read_size);

	/**
	 * Read a record from DCServer using recordname
	*/
	err_t ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size);

	/**
	 * Read payload only
	*/
	err_t ReadRecordData(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size);

	/**
	 * Ask DCFS middleware to write a contiguous block to the file identified by hashname.
	 * Middleware will handle the details of writing to the correct DataCapsule.
	 * Note for implementation: use async send and return when all the data is written if possible.
	 * 
	*/
	err_t WriteRecord(std::string dcname, std::vector<buf_desc_t> *desc_vec);

	/**
	 * Ask DCFS middleware to allocate a new file DataCapsule and return hashname of it.
	 * We could have reused Write() but this approach is preferred to manage the lifetime of Inode.
	*/
	err_t CreateNewFile(std::string *hashname);

	/**
	 * Ask DCFS middleware to load the root directory of the file system
	*/
	err_t LoadRoot(Directory **root);


private:
	DCFSMid *middleware_;
	DCServer *dcserver_;
};


void init_backend(StorageBackend **backend);
#endif