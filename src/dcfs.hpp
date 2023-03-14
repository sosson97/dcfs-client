#ifndef DCFS_HPP_
#define DCFS_HPP_

#define FD_TABLE_MAX 256
#define INO_TABLE_MAX (10 << 1)

	
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


extern struct dcfs_options options;
#endif