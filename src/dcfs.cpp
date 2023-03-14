/** @file
 *
 * DataCapsule File System Client
 *
 * Compile with:
 *
 *     g++ -Wall dcfs.cpp `pkg-config fuse3 --cflags --libs` -o dcfs
 *
 * ## Source code ##
 * \include dcfs.cpp
 * 
 * ## Mounting options ##
 */

#define FUSE_USE_VERSION 31

// C includes
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>

// C++ includes
#include <vector>
#include "dcfs.hpp"
#include "dir.hpp"
#include "inode.hpp"
#include "backend.hpp"

FILE *logf;

struct dcfs_options options;

#define OPTION(t, p)                           \
    { t, offsetof(struct dcfs_options, p), 1 }
static const struct fuse_opt option_spec[] = {
	//OPTION("--name=%s", filename),
	//OPTION("--contents=%s", contents),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

Directory *root;
uint64_t fd_cnt = 10;
uint64_t fd_table[FD_TABLE_MAX];

static void *dcfs_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	fprintf(logf, "dcfs_init\n");
	fflush(logf);
	(void) conn;
	//cfg->auto_cache = 1;
	root = new Directory();
	memset(fd_table, 0, sizeof(uint64_t) * FD_TABLE_MAX);

	init_inode();
	fprintf(logf, "init_inode\n");
	fflush(logf);

	init_backend();

	// todo: setup connection w/ middleware and DC storage servers
	fprintf(logf, "done\n");
	fflush(logf);
	return NULL;
}

static int dcfs_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	int res = 0;
	DirectoryEntry *ent;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;	
	} else {
		res = -ENOENT;
		root->init_readdir();
		while(ent = root->readdir()) {
			if (strcmp(path+1, ent->name().c_str()) == 0) {
				auto inode = get_inode(ent->ino());
				stbuf->st_mode = S_IFREG | 0444;
				stbuf->st_nlink = 1;
				stbuf->st_size = inode->size();
				res = 0;
				break;
			}
		}
	}

	return res;
}

static int dcfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	/* Currently, only supports rootdir*/
	if (strcmp(path, "/") != 0) 
		return -ENOENT;
	
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	DirectoryEntry *ent;
	root->init_readdir();
	while(ent = root->readdir()) {
		filler(buf, ent->name().c_str(), NULL, 0, 0);
	}

	return 0;
}
static int dcfs_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi) {
	DirectoryEntry *ent;
	Inode *inode;
	uint64_t fd;

	root->init_readdir();
	while(ent = root->readdir()) {
		if (strcmp(path+1, ent->name().c_str()) == 0)
			return -EEXIST;
	}
	
	inode = allocate_inode();
	ent = new DirectoryEntry(inode->inum(), std::string(path+1));

	root->addent(ent);

	fd = ++fd_cnt;
	fd_table[fd] = ent->ino(); 	
	fi->fh = fd;

	return 0;
}

static int dcfs_open(const char *path, struct fuse_file_info *fi)
{
	/* Currently, only supports rootdir*/
	DirectoryEntry *ent;
	uint64_t fd;
	root->init_readdir();
	while(ent = root->readdir()) {
		if (strcmp(path+1, ent->name().c_str()) == 0) {
			break;
		}
	}
	if (ent == NULL)
		return -ENOENT;

	fd = ++fd_cnt;
	fd_table[fd] = ent->ino(); 	
	fi->fh = fd;
	// could be used later
	//if ((fi->flags & O_ACCMODE) != O_RDONLY)
	//	return -EACCES;

	return 0;
}

static int dcfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	Inode *inode;

	(void) fi;
	//if(strcmp(path+1, options.filename) != 0)
	//	return -ENOENT;

	if (fi->fh > 0)
		inode = get_inode(fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		root->init_readdir();
		while(ent = root->readdir()) {
			if (strcmp(path+1, ent->name().c_str()) == 0) {
				inode = get_inode(ent->ino());
				break;
			}
		}
	}
	
	if (!inode)
		return -ENOENT;


	len = inode->size();
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;	
		inode->read(buf, offset, size);
	} else
		size = 0;

	return size;
}

static int dcfs_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	Inode *inode = NULL;

	(void) fi;
	//if(strcmp(path+1, options.filename) != 0)
	//	return -ENOENT;
	if (fi->fh > 0)
		inode = get_inode(fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		root->init_readdir();
		while(ent = root->readdir()) {
			if (strcmp(path+1, ent->name().c_str()) == 0) {
				inode = get_inode(ent->ino());
				break;
			}
		}
	}

	if (!inode)
		return -ENOENT;


	inode->write(buf, offset, size);

	return size;
}





static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "\n");
}


static const struct fuse_operations dcfs_oper = {
	.getattr	= dcfs_getattr,
	.open		= dcfs_open,
	.read		= dcfs_read,
	.write		= dcfs_write,
	.readdir	= dcfs_readdir,
	.init           = dcfs_init,
	.create = dcfs_create,
};


int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	logf = fopen("/tmp/dcfs.log", "a");

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	options.block_size_in_kb = 16;
	//options.contents = strdup("dcfs World!\n");

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}

	fprintf(logf, "before fuse_main\n", ret);
	fflush(logf);
	ret = fuse_main(args.argc, args.argv, &dcfs_oper, NULL);
	fprintf(logf, "%d\n", ret);
	fflush(logf);
	fuse_opt_free_args(&args);
	return ret;
}
