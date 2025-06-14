#ifndef K_FS_EXT2_H
#define K_FS_EXT2_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define EXT2_SUPER_MAGIC     0xEF53
#define EXT2_SUPER_OFFSET    1024
#define EXT2_ROOT_INO        2 // Root directory inode is always 2

typedef struct __attribute__((packed))
{
    uint32_t s_inodes_count;         /* Total inodes */
    uint32_t s_blocks_count;         /* Total blocks */
    uint32_t s_r_blocks_count;       /* Reserved blocks */
    uint32_t s_free_blocks_count;    /* Free blocks */
    uint32_t s_free_inodes_count;    /* Free inodes */
    uint32_t s_first_data_block;     /* First data block */
    uint32_t s_log_block_size;       /* Block size = 1024 << this */
    uint32_t s_log_frag_size;        /* Fragment size */
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;                /* Mount time */
    uint32_t s_wtime;                /* Write time */
    uint16_t s_mnt_count;            /* Mounts since last check */
    uint16_t s_max_mnt_count;        /* Max mounts before check */
    uint16_t s_magic;                /* = EXT2_SUPER_MAGIC */
    uint16_t s_state;                /* FS state */
    uint16_t s_errors;               /* On error behavior */
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;            /* Time of last check */
    uint32_t s_checkinterval;        /* Max time between checks */
    uint32_t s_creator_os;
    uint32_t s_rev_level;            /* 0 = original, 1 = v2 */
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    /* (there are more fields in later revisions) */
}
ext2_superblock_t;

typedef struct __attribute__((packed))
{
    uint16_t i_mode;        /* File type & permissions */
    uint16_t i_uid;         /* Owner UID */
    uint32_t i_size_lo;     /* Low 32 bits of size */
    uint32_t i_atime;       /* Access time */
    uint32_t i_ctime;       /* Creation time */
    uint32_t i_mtime;       /* Modification time */
    uint32_t i_dtime;       /* Deletion time */
    uint16_t i_gid;         /* Group ID */
    uint16_t i_links_count; /* Hard links count */
    uint32_t i_blocks;      /* Blocks (512-byte chunks) */
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[12];   /* Direct pointers */
    uint32_t i_single;      /* Single indirect */
    uint32_t i_double;      /* Double indirect */
    uint32_t i_triple;      /* Triple indirect */
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;     /* Directory ACL or high bits of file size */
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
}
ext2_inode_t;

// A group descriptor is 32 bytes; we only need first fields here
typedef struct __attribute__((packed))
{
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
}
ext2_group_desc_t;

typedef struct
{
    ext2_superblock_t sb;
    uint32_t          block_size;
    ext2_group_desc_t *bgdt;         // in-memory copy of group descriptors (allocated with malloc)
    uint8_t          *image;        // pointer to the raw disk image
}
ext2_fs_t;

typedef struct __attribute__((packed))
{
    uint32_t inode;         // Inode number
    uint16_t rec_len;       // Length of this record
    uint8_t  name_len;      // Length of name
    uint8_t  file_type;     // File type (in EXT2 rev 1)
    char     name[];        // Filename (variable length)
}
ext2_dir_entry_t;

// Mount the image in memory; call once at startup
bool ext2_mount(ext2_fs_t *fs, uint8_t *image_data);

// Read inode # (1-based; root is 2)
bool ext2_read_inode(ext2_fs_t *fs, uint32_t inode_no,
                     ext2_inode_t *out_inode);

// Read a directory: invoke callback for each entry
typedef bool (*ext2_dirent_cb_t)(const char *name, uint32_t inode, void *ctx);
void ext2_read_dir(ext2_fs_t *fs, ext2_inode_t *dir_inode,
                   ext2_dirent_cb_t cb, void *ctx);

// Read file data: reads up to buf_len bytes from offset into out_buf
size_t ext2_read_file(ext2_fs_t *fs, ext2_inode_t *inode,
                      void *out_buf, size_t buf_len, size_t offset);

#endif
