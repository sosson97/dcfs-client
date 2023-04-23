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

// assumption: desc is allocated
err_t StorageBackend::ReadFileMeta(std::string hashname, std::string *recordname, buf_desc_t *desc, uint64_t *read_size) {	
	err_t ret = NO_ERR;

	char *buf = new char[hashname.size()];
	memcpy(buf, hashname.c_str(), hashname.size());
	unsigned char* hash = Util::hash256(buf, hashname.size(), NULL);
	
	int siglen = 0;
	unsigned char * signature = Util::sign(client_key_pair_, hash, SHA256_DIGEST_LENGTH, &siglen);
	if (signature == NULL) {
		return ERR_SIGN;
	}

	delete[] buf;	


	ret = middleware_->GetInodeName(hashname, recordname, signature, siglen);
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
	uint64_t full_read_size;

	alloc_buf_desc(&full_desc, desc->size + RECORD_HEADER_SIZE + SPARE_HASH_SPACE);
	err_t ret = dcserver_->ReadRecord(dcname, recordname, &full_desc, &full_read_size);
	if (ret < 0)
		return ret;
	
	capsule::CapsulePDU pdu;
	pdu.ParseFromArray(full_desc.buf, full_read_size);
	*read_size = pdu.payload_in_transit().size();

	if (*read_size > desc->size)
		return ERR_BUF_TOO_SMALL;
	
	memcpy(desc->buf, pdu.payload_in_transit().data(), *read_size);

	dealloc_buf_desc(&full_desc);

	return NO_ERR;
}

err_t StorageBackend::WriteRecord(std::string dcname, std::vector<buf_desc_t> *descs, std::string inode_recordname, std::string aes_key) {
	if (descs->size() == 0)
		return NO_ERR;

	size_t len = dcname.length() + inode_recordname.length() + aes_key.length();
	for (auto desc : *descs) {
		len += desc.size;
	}

	char *args = new char[len];

	size_t offset = 0;
	memcpy(args + offset, dcname.c_str(), dcname.length());
	offset += dcname.length();
	for (size_t i = 0; i < descs->size(); i++) {
		memcpy(args + offset, descs->at(i).buf, descs->at(i).size);
		offset += descs->at(i).size;
	}
	memcpy(args + offset, inode_recordname.c_str(), inode_recordname.length());
	offset += inode_recordname.length();
	memcpy(args + offset, aes_key.c_str(), aes_key.length());
	
	unsigned char* hash = Util::hash256((void*)args, len, NULL);

	int siglen = 0;
	unsigned char * signature = Util::sign(client_key_pair_, hash, SHA256_DIGEST_LENGTH, &siglen);
	if (signature == NULL) {
		return ERR_SIGN;
	}


	delete[] args;	

	return middleware_->Modify(dcname, descs, inode_recordname, aes_key, signature, siglen);
}	

err_t StorageBackend::CreateNewFile(std::string *hashname, std::string *aes_key) {
	return middleware_->CreateNew(hashname, aes_key, NULL, NULL);
}


// Assumption: root directory name = 00000000...0 (32bytes)
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