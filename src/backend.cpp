#include <fstream>
#include <cstring>
#include <cassert>
#include "backend.hpp"

void init_backend(StorageBackend *backend) {
	backend = new FileBackend(BACKEND_MNT_POINT);
}

void alloc_buf_desc(buf_desc_t *desc, uint64_t size) {
	desc->buf = new char[size];
	desc->size = size;
}

void dealloc_buf_desc(buf_desc_t *desc) {
	delete[] desc->buf;
}

/**
 * Disclaimer: No signature is used in this implementation
*/

err_t StorageBackend::ReadFileMeta(std::string hashname, buf_desc_t *desc) {	
	return middleware_->ReadMeta(hashname, desc, "");
}

err_t StorageBackend::ReadRecord(std::string hashname, buf_desc_t *desc) {
	return dcserver_->ReadRecord(hashname, desc);
}

err_t StorageBackend::WriteRecord(std::string hashname, std::vector<const buf_desc_t> *descs) {
	// not implemented
}	