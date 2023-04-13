#include <fstream>
#include <cstring>
#include <cassert>

#include <chrono>

#include "backend.hpp"
#include "util/crypto.hpp"
#include "util/encode.hpp"


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
err_t DCFSMidSim::composeRecord(record_type type,
					std::vector<std::string> *hashes, 
					buf_desc_t *in_desc, 
					buf_desc_t *out_desc, 
					std::string *hashname){
	assert(out_desc);
	assert(hashname);

	char *record_buf, *header_buf;

	uint64_t record_size = 0; 
	uint64_t header_size = 0;

	// header
	header_size += RECORD_HEADER_SIZE; // 8 * 4

	uint64_t index_to_hashes = header_size;
	uint64_t index_to_data;
	if (hashes)
		index_to_data = hashes->size() * HASHLEN_IN_BYTES + header_size;
	else
		index_to_data = header_size;
	uint64_t ty_uint  = type;
	uint64_t time_now =  std::chrono::system_clock::now().time_since_epoch().count();

	header_buf = new char[header_size];

	memcpy(header_buf, &index_to_hashes, 8);
	memcpy(header_buf + 8, &index_to_data, 8);
	memcpy(header_buf + 16, &ty_uint, 8);
	memcpy(header_buf + 24, &time_now, 8);


	buf_desc_t desc;
	desc.buf = header_buf;
	desc.size = header_size;

	unsigned char hash_buf[HASHLEN_IN_BYTES];
	if(!Util::hash256((void *)&(desc.buf), desc.size, hash_buf)) {
		return ERR_HASH;
	}
	*hashname = std::string((char *)hash_buf, HASHLEN_IN_BYTES);		

	record_size += header_size;

	//hashes
	if (hashes)
		record_size += hashes->size() * HASHLEN_IN_BYTES;

	//data
	if (in_desc)
		record_size += in_desc->size;


	record_buf = new char[record_size];
	
	memcpy(record_buf, header_buf, header_size);
	
	uint64_t offset = header_size;
	if (hashes) {
		for(auto v: *hashes) {
			memcpy(record_buf + offset, v.c_str(), v.size());
			offset += v.size();
		}
	}
	
	if (in_desc)
		memcpy(record_buf + offset, in_desc->buf, in_desc->size);

	out_desc->buf = record_buf;
	out_desc->size = record_size;

	delete[] header_buf;

	return NO_ERR;
}

err_t DCFSMidSim::CreateNew(std::string *hashname) {
	buf_desc_t desc;
	composeRecord(META, NULL, NULL, &desc, hashname);
	
	err_t err = dcserver_->WriteRecord(*hashname,
		*hashname,
		&desc);	
	if (err < 0)
		return err;

	index_[*hashname] = "";

	dealloc_buf_desc(&desc);

	return NO_ERR;
}

err_t DCFSMidSim::GetRoot(std::string *hashname, std::string *recordname) {	
	*hashname = root_.first;
	*recordname = root_.second;
	return NO_ERR;
}

err_t DCFSMidSim::GetInodeName(std::string hashname, std::string *recordname) {
	if (index_.find(hashname) == index_.end()) {
		// TODO: call freshness service if not available
		return ERR_NOT_FOUND;
	}
	
	*recordname = index_[hashname];
	return NO_ERR;
}

err_t DCFSMidSim::Modify(std::string dcname, const std::vector<buf_desc_t> *descs) {
	std::vector<std::pair<uint64_t, std::string>> new_data_blocks;
	err_t ret;
	InodeRecord inode_record;
	BlockMapRecord blockmap_record;

	if (index_.find(dcname) == index_.end())
		return ERR_NOT_FOUND;
	
	if (index_[dcname] != "") {	
		// read the inode record
		buf_desc_t record_desc;
		uint64_t record_size, index_to_data;
		record_desc.size = MAX_INODE_RECORD_SIZE; 
		record_desc.buf = new char[record_desc.size];

		ret = dcserver_->ReadRecord(dcname, index_[dcname], &record_desc, &record_size);
		if (ret < 0)
			return ret;
		memcpy(&index_to_data, record_desc.buf + 8, 8);
		memcpy(&inode_record.isize, record_desc.buf + index_to_data, 8);
		inode_record.blockmap_hash = std::string(record_desc.buf + index_to_data + 8, HASHLEN_IN_BYTES);

		// read the blockmap record
		record_desc.size = MAX_BLOCKMAP_RECORD_SIZE;
		delete [] record_desc.buf;
		record_desc.buf = new char[record_desc.size];
		ret = dcserver_->ReadRecord(dcname, inode_record.blockmap_hash, &record_desc, &record_size);
		if (ret < 0)
			return ret;

		blockmap_record.hash_to_latest_data_block = std::string(
										record_desc.buf + RECORD_HEADER_SIZE + HASHLEN_IN_BYTES, 
										HASHLEN_IN_BYTES);

		uint64_t offset = RECORD_HEADER_SIZE;
		while (offset < record_size) {
			std::string hashname;
			hashname = std::string(record_desc.buf + offset, HASHLEN_IN_BYTES);
			blockmap_record.data_hashes.push_back(hashname);
			offset += HASHLEN_IN_BYTES;
		}

		dealloc_buf_desc(&record_desc);
	}

	std::string data_block_hashname = "";
	for (auto desc: *descs) {
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
		data_desc.size = sizeof(uint64_t) + HASHLEN_IN_BYTES;
		data_desc.buf = new char[data_desc.size];
		memcpy(data_desc.buf, &inode_record.isize, sizeof(uint64_t));
		memcpy(data_desc.buf + sizeof(uint64_t), inode_record.blockmap_hash.c_str(), HASHLEN_IN_BYTES);

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