#include <fstream>
#include <cstring>
#include <cassert>
#include "backend.hpp"

class DCFSMidSim : public DCFSMid {
public:
	;

};


/* DCServer simulation using local file
*  Files are stored in the following format in local file system:
*  [file hashname]-inode
*  [file hashname]-blockmap
*  [file hashname]-datablock
*/
class DCServerSim : public DCServer {
	err_t ReadRecord(std::string recordname, buf_desc_t *desc);
	err_t WriteRecord(std::string recordname, const buf_desc_t *desc);			
};


static std::string get_last_line(std::string filename) {
	std::ifstream in(filename);
	std::string line;
	while (std::getline(in, line)) {
	}
	return line;
}

static void append_line(std::string filename, std::string line) {
	FILE *fp = fopen(filename.c_str(), "a");
	if (fp == NULL) {
		return;
	}
	fprintf(fp, "\n%s", line.c_str());
	fclose(fp);
}


/* File backend
*  Files are stored in the following format in local file system:
*  [file hashname]-inode
*  [file hashname]-blockmap
*  [file hashname]-datablock
*/

class FileBackend: public StorageBackend {
public:
	FileBackend(std::string mnt_point): mnt_point_(mnt_point) {
		fs::create_directories(BACKEND_MNT_POINT);
	}
	~FileBackend() {}

	int read(std::string hashname, void *buf, uint64_t size);
	int write(const void *buf, uint64_t size);
	int allocate_inode();
	int get_inode(std::string hashname, void *buf, uint64_t size);
	int load_root(Directory *root);

private:
	std::string mnt_point_;
};
