#include <fstream>
#include <cstring>
#include <cassert>
#include "backend.hpp"

void init_backend(StorageBackend **backend) {
	*backend = new StorageBackend(BACKEND_MNT_POINT);
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

err_t StorageBackend::ReadFileMeta(std::string hashname, std::string *recordname, buf_desc_t *desc, uint64_t *read_size) {	
	err_t ret = NO_ERR;
	ret = middleware_->GetInodeName(hashname, recordname);
	if (ret < 0)
		return ret;
	ret = dcserver_->ReadRecord(hashname, *recordname, desc, read_size);

	return ret;
}

err_t StorageBackend::ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size) {
	err_t ret = dcserver_->ReadRecord(dcname, recordname, desc, read_size);
	if (ret < 0)
		return ret;
	
	if (*read_size > desc->size)
		return ERR_BUF_TOO_SMALL;
	
	return NO_ERR;
}

err_t StorageBackend::ReadRecordData(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size) {
	buf_desc_t full_desc;
	full_desc.size = desc->size + RECORD_HEADER_SIZE + SPARE_HASH_SPACE;
	full_desc.buf = new char[full_desc.size];

	uint64_t full_read_size;
	err_t ret = dcserver_->ReadRecord(dcname, recordname, &full_desc, &full_read_size);
	if (ret < 0)
		return ret;
	
	uint64_t index_to_data;
	memcpy(&index_to_data, full_desc.buf, sizeof(uint64_t));
	*read_size = full_read_size - index_to_data;

	if (*read_size > desc->size)
		return ERR_BUF_TOO_SMALL;
	memcpy(desc->buf, full_desc.buf + index_to_data, full_read_size - index_to_data);


	dealloc_buf_desc(&full_desc);

	return NO_ERR;
}

err_t StorageBackend::WriteRecord(std::string dcname, std::vector<buf_desc_t> *descs) {
	return middleware_->Modify(dcname, descs);
}	

err_t StorageBackend::CreateNewFile(std::string *hashname) {
	return middleware_->CreateNew(hashname);
}

err_t StorageBackend::LoadRoot(Directory **root) {
	*root = new Directory();
	return NO_ERR; // do nothing for now. Let's test without thi
	/*
	err_t ret = NO_ERR;
	std::string hashname, recordname;	
	ret = middleware_->GetRoot(&hashname, &recordname);
	if (ret < 0)
		return ret;

	buf_desc_t desc;
	uint64_t read_size;
	alloc_buf_desc(&desc, MAX_RECORD_SIZE);
	ret = dcserver_->ReadRecord(hashname, recordname, &desc, &read_size);
	if (ret < 0)
		return ret;

	// not implemented yet
	load_directory(root, desc.buf, read_size);*/
}