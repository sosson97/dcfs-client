#ifndef DCFS_HPP_
#define DCFS_HPP_

#include "dir.hpp"	
#include "backend.hpp"
#include "const.hpp"
/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
struct dcfs_options {
	uint64_t block_size_in_kb;
	int show_help;
};

struct Conn {
	int unimpl;
};

struct DCFS {
	DCFS(): root(NULL), backend(NULL) {}
	Directory *root;
	StorageBackend *backend;

	Conn mid_conn;
	Conn dc_conn;
};

struct SuperBlock {
	uint64_t block_size_in_kb;
	char root_addr[32];
};



extern struct dcfs_options options;
#endif