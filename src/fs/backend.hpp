#ifndef BACKEND_HPP_
#define BACKEND_HPP_

#include <vector>
#include <map>
#include <filesystem>

#include <stdint.h>
#include <cassert>
#include <thread>
#include <openssl/evp.h>

#include "const.hpp"
#include "dir.hpp"

#include "dc-client/dc_client.hpp"
#include "util/crypto.hpp"
/* local file but xx style*/

enum record_type {
	META,
	DIRECTORY,
	INODE,
	BLOCKMAP,
	DATABLOCK,
};


inline std::string record_type_to_string(record_type type) {
	switch (type) {
		case META:
			return "META";
		case DIRECTORY:
			return "DIRECTORY";
		case INODE:
			return "INODE";
		case BLOCKMAP:
			return "BLOCKMAP";
		case DATABLOCK:
			return "DATABLOCK";
		default:
			assert(0);
	}
}

#define SPARE_HASH_SPACE (32 * 10) // 10 hashes
#define RECORD_HEADER_SIZE (256)
#define MAX_INODE_RECORD_SIZE (1024) 
#define MAX_BLOCKMAP_RECORD_SIZE (1024 + 32 * BLOCKMAP_COVER)


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
	virtual err_t CreateNew(std::string *hashname,  const unsigned char *sig, size_t siglen) = 0;
	virtual err_t Modify(std::string hashname,
				const std::vector<buf_desc_t> *desc_vec, std::string inode_hash, const unsigned char *sig, size_t siglen) = 0;

	// client expect MW gives the record name of the latest inode record.
	virtual err_t GetInodeName(std::string hashname, 
				std::string *recordname,  const unsigned char *sig, size_t siglen) = 0;

	virtual err_t GetRoot(std::string *hashname, std::string *recordname, const unsigned char *sig, size_t sigle) = 0;
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

	DCFSMidSim(DCServer *dcserver) : dcserver_(dcserver) {
		middlewareWriterKey_ = Util::generate_evp_pkey_dsa();
		Util::generate_symmetric_key(symmetric_middleware_key_);
	}
	// DCFSMidSim(DCServer *dcserver);

	err_t CreateNew(std::string *hashname, const unsigned char *sig, size_t siglen);
	err_t GetRoot(std::string *hashname, std::string *recordname, const unsigned char *sig, size_t siglen);
 	err_t GetInodeName(std::string hashname, std::string *recordname, const unsigned char *sig, size_t siglen);
	err_t Modify(std::string dcname, const std::vector<buf_desc_t> *descs, std::string inode_hash, const unsigned char *sig, size_t siglen);
private:
	struct InodeRecord {
		InodeRecord() : isize(0), blockmap_hash("") {}

		uint64_t isize;
		std::string blockmap_hash;
		char key[16];
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
	EVP_PKEY *middlewareWriterKey_;
	unsigned char symmetric_middleware_key_[16];
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

class DCServerNet : public DCServer {
public:
	/* Server IP address, port, and other configs are defined in dc_config.hpp.
	 * So we have nothind to do here.
	 * I know it's a weird design, but let's just keep it to use legacy code.
	*/
	DCServerNet(); 
	~DCServerNet();
	err_t ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size);
	err_t WriteRecord(std::string dcname, std::string recordname, const buf_desc_t *desc);

private:
	DCClient *dcclient_;
	std::thread *listen_thread_;
	std::atomic<bool> end_signal_;	
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
		dcserver_ = new DCServerNet();
		//dcserver_ = new DCServerSim(mnt_point);
		middleware_ = new DCFSMidSim(dcserver_);
	}

	~StorageBackend() {
		delete dcserver_;
		delete middleware_;
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