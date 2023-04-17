#include "errno.hpp"
#include "backend.hpp"
#include "util/logging.hpp"
#include "util/encode.hpp"
#include "dc-client/dc_config.hpp"

#include <thread>
 /**
  *  dcname = hashname of dcmeta record. 
  *  no need to use dcname given that we are using a single DC server
 */

DCServerNet::DCServerNet() {  
	dcclient_ = new DCClient(CLIENT_ID);
	end_signal_ = false;


	listen_thread_ = new std::thread(&DCClient::RunListenServer, dcclient_, &end_signal_);	
}

DCServerNet::~DCServerNet() {
	end_signal_ = true;
	listen_thread_->join();
	delete dcclient_;
}

err_t DCServerNet::ReadRecord(std::string dcname, std::string recordname, buf_desc_t *desc, uint64_t *read_size) {
	std::string *out = dcclient_->Get(recordname, DCGetOptions{false, false});
	if (out == NULL) {
		Logger::log(ERROR, "DCServerNet::ReadRecord: Failed to read record" 
				+ Util::binary_to_hex_string(recordname.c_str(), recordname.length()) 
				+ "from DC server");
		return ERR_IO;
	}

	assert(out->length() <= desc->size);

	memcpy(desc->buf, out->c_str(), out->length());
	*read_size = out->length();

	return NO_ERR;
}


err_t DCServerNet::WriteRecord(std::string dcname, std::string recordname, const buf_desc_t *desc) {
	dcclient_->Put(recordname, std::string(desc->buf, desc->size));

	return NO_ERR;
}