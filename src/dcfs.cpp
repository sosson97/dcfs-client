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

DCFS *dcfs;
uint64_t fd_cnt = 10;
std::string fd_table[FD_TABLE_MAX];

static void *dcfs_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	fprintf(logf, "dcfs_init\n");
	fflush(logf);
	(void) conn;
	//cfg->auto_cache = 1;
	
	dcfs = new DCFS();	
	init_inode();
	init_backend(dcfs->backend);
	dcfs->backend->load_root(dcfs->root);
	
	for (int i = 0; i < FD_TABLE_MAX; i++)
		fd_table[i] = "";
	// todo: setup connection w/ middleware and DC storage servers
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
		dcfs->root->init_readdir();
		while(ent = dcfs->root->readdir()) {
			if (strcmp(path+1, ent->filename().c_str()) == 0) {
				auto inode = get_inode(dcfs, ent->hashname());
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

	/* Currently, only supports dcfs->rootdir*/
	if (strcmp(path, "/") != 0) 
		return -ENOENT;
	
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	DirectoryEntry *ent;
	dcfs->root->init_readdir();
	while(ent = dcfs->root->readdir()) {
		filler(buf, ent->filename().c_str(), NULL, 0, 0);
	}

	return 0;
}
static int dcfs_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi) {
	DirectoryEntry *ent;
	Inode *inode;
	uint64_t fd;

	dcfs->root->init_readdir();
	while(ent = dcfs->root->readdir()) {
		if (strcmp(path+1, ent->filename().c_str()) == 0)
			return -EEXIST;
	}
	
	inode = allocate_inode(dcfs);
	inode->ref();
	ent = new DirectoryEntry(std::string(path+1), inode->hashname());

	dcfs->root->addent(ent);

	fd = ++fd_cnt;
	fd_table[fd] = ent->hashname(); 	
	fi->fh = fd;

	return 0;
}

static int dcfs_open(const char *path, struct fuse_file_info *fi)
{
	/* Currently, only supports dcfs->rootdir*/
	DirectoryEntry *ent;
	uint64_t fd;
	dcfs->root->init_readdir();
	while(ent = dcfs->root->readdir()) {
		if (strcmp(path+1, ent->filename().c_str()) == 0) {
			break;
		}
	}
	if (ent == NULL)
		return -ENOENT;

	Inode *ino = get_inode(dcfs, ent->hashname());
	ino->ref();	

	fd = ++fd_cnt;
	fd_table[fd] = ent->hashname(); 	
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
		inode = get_inode(dcfs, fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		dcfs->root->init_readdir();
		while(ent = dcfs->root->readdir()) {
			if (strcmp(path+1, ent->filename().c_str()) == 0) {
				inode = get_inode(dcfs, ent->hashname());
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
		inode = get_inode(dcfs, fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		dcfs->root->init_readdir();
		while(ent = dcfs->root->readdir()) {
			if (strcmp(path+1, ent->filename().c_str()) == 0) {
				inode = get_inode(dcfs, ent->hashname());
				break;
			}
		}
	}

	if (!inode)
		return -ENOENT;


	inode->write(buf, offset, size);

	return size;
}

static int dcfs_release(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;

	/* Currently, only supports dcfs->rootdir*/
	DirectoryEntry *ent;
	uint64_t fd;
	dcfs->root->init_readdir();
	while(ent = dcfs->root->readdir()) {
		if (strcmp(path+1, ent->filename().c_str()) == 0) {
			break;
		}
	}
	if (ent == NULL)
		return -ENOENT;

	Inode *ino = get_inode(dcfs, ent->hashname());
	ino->unref();		

	return 0;
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
	.release = dcfs_release,
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
	options.block_size_in_kb = DEFAULT_BLOCK_SIZE_IN_KB;
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
