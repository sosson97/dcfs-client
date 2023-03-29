#ifndef CONST_HPP_
#define CONST_HPP_

#define BACKEND_MNT_POINT "/tmp/dcfs"
#define DEFAULT_BLOCK_SIZE_IN_KB 16
#define HASHLEN_IN_BYTES 32

#define FD_TABLE_MAX 256
#define SUPERBLOCK_NAME "superblock"

#define BLOCKMAP_SIZE_IN_KB 16
#define BLOCKMAP_COVER (BLOCKMAP_SIZE_IN_KB * 1024 / HASHLEN_IN_BYTES)

#define MAX_FILEMETA_SIZE 1024 * 4

#endif // CONST_HPP_