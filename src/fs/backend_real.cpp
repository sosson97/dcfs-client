#include "errno.hpp"
#include "backend.hpp"
#include "util/logging.hpp"
#include "util/encode.hpp"

 /**
  *  dcname = hashname of dcmeta record. 
  *  no need to use dcname given that we are using a single DC server
 */

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
}