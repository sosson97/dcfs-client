#include <fstream>
#include <cstring>
#include <cassert>

#include <chrono>

#include "backend.hpp"
#include "util/crypto.hpp"
#include "util/encode.hpp"
#include "util/options.hpp"
#include "util/logging.hpp"
#include "dc-client/dc_config.hpp"

/**
 * Temporary Record Format
 * 0 -- index to hash starts, index to data starts
 * HEADER (record name = header hash)
 * 1 -- TYPE
 * 2 -- TIME CREATED
 * ---
 * 3 -- HASHES
 * 3+N -- HASHES, END
 * ---
 * 3+N+1 -- DATA
*/

/** Temporary InodeRecord Data Format
 * HASH1 = previous inode
 * HASH2 = latest blockmap at the moment of writing
 * ---
 * ISIZE(8bytes)  BLOCKMAP HASH(32bytes)
*/

/** Temporary BlockMapRecord Data Format
 * HASH1 = previous blockmap
 * HASH2 = latest data block at the moment of writing
 * ---
 * 32bytes HASH to first data block (0-16kb)
 * 32bytes HASH to second data block (16kb-32kb)
 * ...
 */


/**
 * DataCapsule Payload Format
 * Hash, time created, type will be moved to DataCapsule header
 * 
 * DataRecord
 * HEADER
 * - prevHash = previous DataRecord
 * - payload hash
 * - time created
 * - type = DATA
 * - replayaddr
 * PAYLOAD (16KB)
 * - DATA
 * 
 * InodeRecord
 * HEADER
 * - prevHash 1 = previous inode record
 * - prevHash 2 = latest blockmap record
 * - payload hash
 * - time created
 * - type = INODE
 * - replayaddr
 * PAYLOAD 
 * - ISIZE(8bytes)
 * - AES_KEY(16bytes)
 * 
 * BlockMapRecord
 * - prevHash 1 = previous blockmap record
 * - prevHash 2 = latest data record
 * - payload hash
 * - time created
 * - type = BLOCKMAP
 * - replayaddr
 * PAYLOAD (16KB)
 * - 32bytes HASH to first data block (0-16kb)
 * - ...
*/

// Temporary hardcoded variable
//EVP_PKEY *client_key_pair_ = Util::generate_evp_pkey_dsa();


err_t DCFSMidSim::composeRecord(record_type type,
					std::vector<std::string> *hashes, 
					buf_desc_t *in_desc, 
					buf_desc_t *out_desc, 
					std::string *hashname){
	assert(out_desc);
	assert(hashname);


	capsule::CapsulePDU *pdu = new capsule::CapsulePDU();

	pdu->mutable_header()->set_sender(0);
	
	if (hashes) {
		for (auto hash : *hashes) {
			pdu->mutable_header()->add_prevhash(hash);
		}
	}

	if (in_desc) {
		buf_desc_t desc;
		desc.buf = in_desc->buf;
		desc.size = in_desc->size;

		unsigned char hash_buf[HASHLEN_IN_BYTES];
		if(!Util::hash256((void *)desc.buf, desc.size, hash_buf)) {
			return ERR_HASH;
		}
		pdu->mutable_header()->set_hash(std::string((char *)hash_buf, HASHLEN_IN_BYTES));
	} else {
		pdu->mutable_header()->set_hash("0");
	}
	pdu->mutable_header()->set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());
	pdu->mutable_header()->set_msgtype(record_type_to_string(type));
	pdu->mutable_header()->set_replyaddr(Util::load_client_ip() + std::string(":") + std::to_string(NET_CLIENT_RECV_ACK_PORT + CLIENT_ID));

	buf_desc_t desc;
	alloc_buf_desc(&desc, pdu->header().ByteSizeLong());
	pdu->header().SerializeToArray(desc.buf, pdu->header().ByteSizeLong());

	unsigned char hash_buf[HASHLEN_IN_BYTES];
	if(!Util::hash256((void *)desc.buf, desc.size, hash_buf)) {
		return ERR_HASH;
	}
	*hashname = std::string((char *)hash_buf, HASHLEN_IN_BYTES);
	pdu->set_header_hash(*hashname);
	dealloc_buf_desc(&desc);

	pdu->set_signature("0"); // TODO: sign
	if (in_desc)
		pdu->set_payload_in_transit(in_desc->buf, in_desc->size);

	out_desc->size = pdu->ByteSizeLong(); 
	out_desc->buf = new char[out_desc->size];
	pdu->SerializeToArray(out_desc->buf, out_desc->size);

	Logger::log(LDEBUG, "composeRecord called for " + record_type_to_string(type));
	Logger::log(LDEBUG, "composeRecord: hashname = " + Util::binary_to_hex_string(hashname->c_str(), hashname->size()));
	for (auto hash : pdu->header().prevhash()) {
		Logger::log(LDEBUG, "composeRecord: prevhash = " + Util::binary_to_hex_string(hash.c_str(), hash.size()));
	}

	return NO_ERR;
}

err_t DCFSMidSim::CreateNew(std::string *hashname, std::string *aes_key, const unsigned char *sig, size_t siglen) {	
	buf_desc_t desc;
	composeRecord(META, NULL, NULL, &desc, hashname);
		
	err_t err = dcserver_->WriteRecord(*hashname,
		*hashname,
		&desc);	
	if (err < 0)
		return err;

	index_[*hashname] = "";

	dealloc_buf_desc(&desc);
	unsigned char aes_key_buf[AES_KEY_LEN];
	Util::generate_symmetric_key(aes_key_buf);
	*aes_key = std::string((char *)aes_key_buf, AES_KEY_LEN);

	return NO_ERR;
}

err_t DCFSMidSim::GetRoot(std::string *hashname, std::string *recordname, const unsigned char *sig, size_t siglen) {
	*hashname = root_.first;
	*recordname = root_.second;
	return NO_ERR;
}

err_t DCFSMidSim::GetInodeName(std::string hashname, std::string *recordname, const unsigned char *sig, size_t siglen) {
	char *buf = new char[hashname.size()];
	memcpy(buf, hashname.c_str(), hashname.size());
	unsigned char* hash = Util::hash256(buf, hashname.size(), NULL);
	if (!Util::verify(client_key_pair_, hash, SHA256_DIGEST_LENGTH, sig, siglen)) {
		return ERR_VERIFY;
	}

	delete[] buf;	

	if (index_.find(hashname) == index_.end()) {
		// TODO: call freshness service if not available
		return ERR_NOT_FOUND;
	}
	
	*recordname = index_[hashname];
	return NO_ERR;
}

err_t DCFSMidSim::Modify(std::string dcname, const std::vector<buf_desc_t> *descs, std::string inode_hash, std::string aes_key, const unsigned char *sig, size_t siglen) {	
	// verify arguments
	size_t len = dcname.length() + inode_hash.length() + aes_key.length();
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
	memcpy(args + offset, inode_hash.c_str(), inode_hash.length());
	offset += inode_hash.length();
	memcpy(args + offset, aes_key.c_str(), aes_key.length());
	
	unsigned char* hash = Util::hash256((void*)args, len, NULL);

	/* Compute the hash of arg*/
	if (!Util::verify(client_key_pair_, hash, SHA256_DIGEST_LENGTH, sig, siglen)) {
		return ERR_VERIFY;
	}

	delete[] args;

	/* Check latest inode hash. If there is no latest inode hash, then assume this is the first modify. */
	if (inode_hash != index_[dcname])  {
		return -1; //CHANGE THE ERROR CODE: TODO
	}

	std::vector<std::pair<uint64_t, std::string>> new_data_blocks;
	err_t ret;
	InodeRecord inode_record;
	BlockMapRecord blockmap_record;

	if (index_.find(dcname) == index_.end())
		return ERR_NOT_FOUND;

	/**
	 * Read the necessary inode record, blockmap record
	*/

	//unsigned char key[16];

	if (index_[dcname] != "") {	// inode record exist
		// read the inode record
		buf_desc_t record_desc;
		uint64_t record_size;		
		alloc_buf_desc(&record_desc, MAX_INODE_RECORD_SIZE);		
		ret = dcserver_->ReadRecord(dcname, index_[dcname], &record_desc, &record_size);
		if (ret < 0)
			return ret;
		capsule::CapsulePDU ino_pdu;
		ino_pdu.ParseFromArray(record_desc.buf, record_size);
		inode_record.blockmap_hash = ino_pdu.header().prevhash(1);

		memcpy(inode_record.key, ino_pdu.payload_in_transit().c_str() + INODE_AES_KEY_OFFSET, AES_KEY_LEN);
		//memcpy(key, inode_record.key, 16);

		// read the blockmap record
		dealloc_buf_desc(&record_desc);
		alloc_buf_desc(&record_desc, MAX_BLOCKMAP_RECORD_SIZE);
		ret = dcserver_->ReadRecord(dcname, inode_record.blockmap_hash, &record_desc, &record_size);
		if (ret < 0)
			return ret;

		capsule::CapsulePDU bm_pdu;
		bm_pdu.ParseFromArray(record_desc.buf, record_size);
		blockmap_record.hash_to_latest_data_block = bm_pdu.header().prevhash(1);

		uint64_t offset = 0;	
		while (offset < bm_pdu.payload_in_transit().size()) {
			std::string hashname;
			hashname = std::string(bm_pdu.payload_in_transit().data() + offset, HASHLEN_IN_BYTES);
			blockmap_record.data_hashes.push_back(hashname);
			offset += HASHLEN_IN_BYTES;
		}

		dealloc_buf_desc(&record_desc);
	} else {
		memcpy(inode_record.key, aes_key.c_str(), AES_KEY_LEN);
	}
	// else { 
	//	memcpy(key, aes_key.c_str(), AES_KEY_LEN);
	//}


	// Push order: data blocks -> blockmap -> inode

	// create and push new data blocks
	std::string data_block_hashname = "";

	// encrypt everything in descs

	for (auto desc: *descs) {
		/*
		char* encrypted_buf = new char[desc.size];
		char iv[16];
		if (!Util::generate_symmetric_key(iv)) {
			return -1; //UJJAINI FIX ERROR
		};
		// MILES: outlen should be something else?
		if (!Util::encrypt_symmetric(key, iv, desc.buf, desc.size, encrypted_buf, desc.size)) {
			return -1; //UJJAINI FIX ERROR TODO
		}
		desc.buf = encrypted_buf;
		*/
		std::vector<std::string> new_data_block_hashes;
		if (data_block_hashname == "") {
			if (blockmap_record.hash_to_latest_data_block != "")
				new_data_block_hashes.push_back(blockmap_record.hash_to_latest_data_block);
			else
				new_data_block_hashes.push_back(dcname); // points to the DC meta record if this is the first data block	
		}
		else
			new_data_block_hashes.push_back(data_block_hashname);

		buf_desc_t record_desc; 
		ret = composeRecord(DATABLOCK, &new_data_block_hashes, &desc, &record_desc, &data_block_hashname);
		if (ret < 0)
			return ret;
		ret = dcserver_->WriteRecord(dcname, data_block_hashname, &record_desc);
		if (ret < 0)
			return ret;
		new_data_blocks.push_back(std::make_pair(desc.file_offset, data_block_hashname));

		dealloc_buf_desc(&record_desc);
	}

	// update the blockmap record
	std::string new_blockmap_hashname;
	{
		std::vector<std::string> new_blockmap_hashes;
		if (inode_record.blockmap_hash != "")
			new_blockmap_hashes.push_back(inode_record.blockmap_hash);
		else
			new_blockmap_hashes.push_back(dcname); // points to the DC meta record if this is the first blockmap record
		new_blockmap_hashes.push_back(new_data_blocks[new_data_blocks.size() - 1].second);

		for (auto block: new_data_blocks) {
			if (block.first < inode_record.isize) 
				blockmap_record.data_hashes[block.first / (DEFAULT_BLOCK_SIZE_IN_KB * 1024)] = block.second;
			else 
				blockmap_record.data_hashes.push_back(block.second); 
		}
		buf_desc_t data_desc;
		data_desc.size = blockmap_record.data_hashes.size() * HASHLEN_IN_BYTES;
		data_desc.buf = new char[data_desc.size];
		uint64_t offset = 0;
		for (auto hash: blockmap_record.data_hashes) {
			memcpy(data_desc.buf + offset, hash.c_str(), HASHLEN_IN_BYTES);
			offset += HASHLEN_IN_BYTES;
		}

		buf_desc_t record_desc;
		ret = composeRecord(BLOCKMAP, &new_blockmap_hashes, &data_desc, &record_desc, &new_blockmap_hashname);
		if (ret < 0)
			return ret;
		ret = dcserver_->WriteRecord(dcname, new_blockmap_hashname, &record_desc);
		if (ret < 0)
			return ret;

		dealloc_buf_desc(&data_desc);
		dealloc_buf_desc(&record_desc);
	}

	// update inode record
	{		
		std::string new_inode_hashname;
		std::vector<std::string> new_inode_hashes;
		if (index_[dcname] != "")
			new_inode_hashes.push_back(index_[dcname]);
		else
			new_inode_hashes.push_back(dcname); // points to the DC meta record if this is the first inode record
		new_inode_hashes.push_back(new_blockmap_hashname);

		if (inode_record.isize <= descs->size() * DEFAULT_BLOCK_SIZE_IN_KB * 1024) {
			inode_record.isize = descs->size() * DEFAULT_BLOCK_SIZE_IN_KB * 1024;
		}	
		inode_record.blockmap_hash = new_blockmap_hashname;

		buf_desc_t data_desc;
		data_desc.size = INODE_PAYLOAD_SIZE;
		data_desc.buf = new char[INODE_PAYLOAD_SIZE];
		
		memcpy(data_desc.buf, &inode_record.isize, sizeof(uint64_t));

		// UJJAINI: add in the symmetric encrypt key to the inode -- likely needs to be encrypted with middleware sym key
		unsigned char encryped_symmetric_key[AES_KEY_LEN + AES_PAD_LEN];
		int outlen;
		Util::encrypt_symmetric(symmetric_middleware_key_, NULL, (unsigned char *)inode_record.key, AES_KEY_LEN, encryped_symmetric_key, &outlen);
		assert(outlen == AES_KEY_LEN + AES_PAD_LEN);
		memcpy(data_desc.buf + INODE_AES_KEY_OFFSET, encryped_symmetric_key, AES_KEY_LEN + AES_PAD_LEN);

		buf_desc_t record_desc;
		ret = composeRecord(INODE, &new_inode_hashes, &data_desc, &record_desc, &new_inode_hashname);
		if (ret < 0)
			return ret;
		ret = dcserver_->WriteRecord(dcname, new_inode_hashname, &record_desc);
		if (ret < 0)
			return ret;
		index_[dcname] = new_inode_hashname;

		dealloc_buf_desc(&data_desc);
		dealloc_buf_desc(&record_desc);
	}

	return NO_ERR;
}

err_t DCFSMidSim::DecryptAESKey(std::string recordname, 
					std::string encrypted_key, 
					std::string *aes_key, 
					const unsigned char *sig, size_t siglen) {
	unsigned char aes_key_buf[AES_KEY_LEN + AES_PAD_LEN];

	int outlen = 0;
	Util::decrypt_symmetric(symmetric_middleware_key_, NULL, (unsigned char *)encrypted_key.c_str(), encrypted_key.length(), aes_key_buf, &outlen);

	assert(outlen == AES_KEY_LEN);

	*aes_key = std::string((char *)aes_key_buf, outlen);

	return NO_ERR;
}



err_t DCServerSim::ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size) {
	dcname = Util::binary_to_hex_string(dcname.c_str(), dcname.length());
	recordname = Util::binary_to_hex_string(recordname.c_str(), recordname.length());

	std::string recordpath = mnt_point_ + "/" + dcname + "/" + recordname;
	std::ifstream in(recordpath, std::ios::binary | std::ios::ate);
	if (!in.is_open()) {
		return ERR_NOT_FOUND;
	}

	std::streamsize size = in.tellg();
	in.seekg(0, std::ios::beg);

	assert(size <= desc->size);

	if (!in.read(desc->buf, size)) {
		return ERR_IO;
	}

	*read_size = size;

	return NO_ERR;
}



//create directory "dcname" if dcname = recordname
err_t DCServerSim::WriteRecord(std::string dcname, std::string recordname, const buf_desc_t *desc) {
	dcname = Util::binary_to_hex_string(dcname.c_str(), dcname.length());
	recordname = Util::binary_to_hex_string(recordname.c_str(), recordname.length());

	std::string dcpath = mnt_point_ + "/" + dcname;
	if (!fs::exists(dcpath)) {
		if (dcname == recordname)
			fs::create_directories(dcpath);
		else
			return ERR_IO;
	}

	std::string recordpath = mnt_point_ + "/" + dcname + "/" + recordname;
	std::ofstream out(recordpath, std::ios::binary);
	if (!out.is_open()) {
		return ERR_IO;
	}

	if (!out.write(desc->buf, desc->size)) {
		return ERR_IO;
	}

	return NO_ERR;
}