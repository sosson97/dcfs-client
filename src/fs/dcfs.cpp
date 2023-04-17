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

#include "util/logging.hpp"
#include "util/options.hpp"


struct dcfs_options options;
//
#define OPTION(t, p)                           \
    { t, offsetof(struct dcfs_options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("--block_size_in_kb=%d", block_size_in_kb),
	OPTION("--client_ip=%s", client_ip),
	OPTION("--dcserver_ip=%s", dcserver_ip),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

DCFS *dcfs;
uint64_t fd_cnt = 10;

/**
 * File descriptor table
 * fd is a handle to the file
 * fd_table[fd] = the hashname of the file
 * you can access to the in-memory Inode of the file using the hashname
 * Backend will take care of caching the necessary data if Inode is not in memory
 * */
std::string fd_table[FD_TABLE_MAX];

static void *dcfs_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	Logger::log(INFO, "DCFS init called");
	(void) conn;
	//cfg->auto_cache = 1;
	

	dcfs = new DCFS();	

	dcfs->block_size_in_kb = options.block_size_in_kb;
	init_inode();
	init_backend(&dcfs->backend);
	Logger::log(INFO, "Backend init finished");

	dcfs->backend->LoadRoot(&dcfs->root); // this does nothing for now
	
	Logger::log(INFO, "Loading root directory finished");

	for (int i = 0; i < FD_TABLE_MAX; i++)
		fd_table[i] = "";
	// todo: setup connection w/ middleware and DC storage servers

	Logger::log(INFO, "DCFS init finished");
	return NULL;
}

static int dcfs_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	Logger::log(LDEBUG, "DCFS getattr called for path: " + std::string(path));

	(void) fi;
	int res = 0;
	DirectoryEntry *ent;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;	
	} else {
		res = -ENOENT;
		dcfs->root->InitReaddir();
		while((ent = dcfs->root->Readdir())) {
			if (strcmp(path+1, ent->Filename().c_str()) == 0) {
				auto inode = get_inode(dcfs, ent->Hashname());
				stbuf->st_mode = S_IFREG | 0444;
				stbuf->st_nlink = 1;
				stbuf->st_size = inode->Size();
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
	Logger::log(LDEBUG, "DCFS readdir called for path: " + std::string(path));

	(void) offset;
	(void) fi;
	(void) flags;

	/* Currently, only supports dcfs->rootdir*/
	if (strcmp(path, "/") != 0) 
		return -ENOENT;
	
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	DirectoryEntry *ent;
	dcfs->root->InitReaddir();
	while((ent = dcfs->root->Readdir())) {
		filler(buf, ent->Filename().c_str(), NULL, 0, 0);
	}

	return 0;
}
static int dcfs_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi) {
	Logger::log(LDEBUG, "DCFS create called for path: " + std::string(path));
				
	DirectoryEntry *ent;
	Inode *inode;
	uint64_t fd;

	dcfs->root->InitReaddir();
	while((ent = dcfs->root->Readdir())) {
		if (strcmp(path+1, ent->Filename().c_str()) == 0)
			return -EEXIST;
	}
	
	inode = allocate_inode(dcfs);
	if (!inode) {
		Logger::log(ERROR, "allocate_inode failed");
		return -ENOMEM;
	}

	inode->Ref();
	ent = new DirectoryEntry(std::string(path+1), inode->Hashname());

	dcfs->root->Addent(ent);

	fd = ++fd_cnt;
	fd_table[fd] = ent->Hashname(); 	
	fi->fh = fd;

	return 0;
}

static int dcfs_open(const char *path, struct fuse_file_info *fi)
{
	Logger::log(LDEBUG, "DCFS open called for path: " + std::string(path));

	/* Currently, only supports dcfs->rootdir*/
	DirectoryEntry *ent;
	uint64_t fd;
	dcfs->root->InitReaddir();
	while((ent = dcfs->root->Readdir())) {
		if (strcmp(path+1, ent->Filename().c_str()) == 0) {
			break;
		}
	}
	if (ent == NULL)
		return -ENOENT;

	Inode *ino = get_inode(dcfs, ent->Hashname());
	if (!ino)
		return -ENOENT;
	
	ino->Ref();	

	fd = ++fd_cnt;
	fd_table[fd] = ent->Hashname(); 	
	fi->fh = fd;
	// could be used later
	//if ((fi->flags & O_ACCMODE) != O_RDONLY)
	//	return -EACCES;

	return 0;
}

static int dcfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	Logger::log(LDEBUG, "DCFS read called for path: " + std::string(path) + " size: " + std::to_string(size) + " offset: " + std::to_string(offset));

	size_t len;
	Inode *inode;

	(void) fi;
	//if(strcmp(path+1, options.filename) != 0)
	//	return -ENOENT;

	if (fi->fh > 0)
		inode = get_inode(dcfs, fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		dcfs->root->InitReaddir();
		while((ent = dcfs->root->Readdir())) {
			if (strcmp(path+1, ent->Filename().c_str()) == 0) {
				inode = get_inode(dcfs, ent->Hashname());
				break;
			}
		}
	}
	
	if (!inode)
		return -ENOENT;


	len = inode->Size();
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		uint64_t read_size;	
		err_t err = inode->Read(buf, offset, size, &read_size);
		if (err < 0) {
			Logger::log(ERROR, "read error:" + err);
			return -EIO;
		}
	} else
		size = 0;

	return size;
}

static int dcfs_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	Logger::log(LDEBUG, "DCFS write called for path: " + std::string(path) + " size: " + std::to_string(size) + " offset: " + std::to_string(offset));
	Inode *inode = NULL;

	(void) fi;
	//if(strcmp(path+1, options.filename) != 0)
	//	return -ENOENT;
	if (fi->fh > 0)
		inode = get_inode(dcfs, fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		dcfs->root->InitReaddir();
		while((ent = dcfs->root->Readdir())) {
			if (strcmp(path+1, ent->Filename().c_str()) == 0) {
				inode = get_inode(dcfs, ent->Hashname());
				break;
			}
		}
	}

	if (!inode)
		return -ENOENT;

	uint64_t write_size;
	err_t err = inode->Write(buf, offset, size, &write_size);
	if (err < 0) {
		Logger::log(ERROR, "write error: " + err);
		return -EIO;
	}

	return size;
}

static int dcfs_release(const char *path, struct fuse_file_info *fi)
{
	Logger::log(LDEBUG, "DCFS release called for path: " + std::string(path));
	(void) path;
	(void) fi;
	//if(strcmp(path+1, options.filename) != 0)
	//	return -ENOENT;
	Inode *inode = NULL;

	if (fi->fh > 0)
		inode = get_inode(dcfs, fd_table[fi->fh]);
	else {
		DirectoryEntry *ent;
		dcfs->root->InitReaddir();
		while((ent = dcfs->root->Readdir())) {
			if (strcmp(path+1, ent->Filename().c_str()) == 0) {
				inode = get_inode(dcfs, ent->Hashname());
				break;
			}
		}
	}
	
	if (!inode)
		return -ENOENT;

	inode->Unref();		

	if (fi->fh > 0)	
		fd_table[fi->fh] = "";
	fi->fh = 0;

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

	if (options.client_ip) {
		Util::option_map["client_ip"] = std::string(options.client_ip);
	}
	if (options.dcserver_ip) {
		Util::option_map["dcserver_ip"] = std::string(options.dcserver_ip);
	}


	ret = fuse_main(args.argc, args.argv, &dcfs_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}
