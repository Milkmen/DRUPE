#include "ext2.h"

#include <string.h>

#include "../../memory/physical.h"

void* kpmalloc(size_t size)
{
    size_t num_sect = 1;
    while(num_sect * PMM_SECTOR_SIZE < size)
        num_sect++;
    return mem_phys_alloc_sectors(num_sect);
}

void kpmemset(void* dst, char v, size_t size)
{
    char* mem = (char*) dst;
    for(int i = 0; i < size; ++i)
    {
        mem[i] = v;
    }
}

void kpmemcpy(void* dst, void* src, size_t size)
{
    char* mem_dst = (char*) dst;
    char* mem_src = (char*) src;
    for(int i = 0; i < size; ++i)
    {
        mem_dst[i] = mem_src[i];
    }
}

bool ext2_mount(ext2_fs_t *fs, uint8_t *image_data) 
{
    fs->image = image_data;

    // 1) Read superblock at byte 1024
    kpmemcpy(&fs->sb,
             image_data + EXT2_SUPER_OFFSET,
             sizeof(ext2_superblock_t));
    if (fs->sb.s_magic != EXT2_SUPER_MAGIC) return false;

    // 2) Compute block size
    fs->block_size = 1024U << fs->sb.s_log_block_size;

    // 3) Read group descriptor table
    //    On 1 KiB blocks it lives in block #2 (bytes 2×1024),
    //    otherwise at the start of block #1.
    uint32_t gd_offset = fs->block_size * ((fs->block_size == 1024) ? 2 : 1);

    uint32_t groups = (fs->sb.s_blocks_count +
                       fs->sb.s_blocks_per_group - 1)
                      / fs->sb.s_blocks_per_group;

    fs->bgdt = (ext2_group_desc_t*)kpmalloc(groups * sizeof(ext2_group_desc_t));
    if (!fs->bgdt) return false;

    kpmemcpy(fs->bgdt,
             image_data + gd_offset,
             groups * sizeof(ext2_group_desc_t));
    return true;
}

#include "../../shell/shell.h"

bool ext2_read_inode(ext2_fs_t *fs, uint32_t inode_no, ext2_inode_t *out_inode)
{
    extern shell_instance_t* g_kernel_shell;
    if (inode_no == 0) return false;
    inode_no--;  

    uint32_t group = inode_no / fs->sb.s_inodes_per_group;
    uint32_t index = inode_no % fs->sb.s_inodes_per_group;
    uint32_t inode_table_block = fs->bgdt[group].bg_inode_table;

    uint32_t inode_size        = sizeof(ext2_inode_t);
    uint32_t inodes_per_block  = fs->block_size / inode_size;
    uint32_t block_offset      = index / inodes_per_block;
    uint32_t inode_offset      = index % inodes_per_block;

    uint32_t abs_offset = 
        (inode_table_block + block_offset) * fs->block_size
      + inode_offset * inode_size;

    kpmemcpy(out_inode,
             fs->image + abs_offset,
             sizeof(ext2_inode_t));
    return true;
}


void ext2_read_dir(ext2_fs_t *fs, ext2_inode_t *dir, ext2_dirent_cb_t cb, void *ctx) 
{
    uint32_t blk_count = dir->i_blocks / (fs->block_size / 512);

    for (uint32_t b = 0; b < 12 && b < blk_count; ++b) 
    {
        if (!dir->i_block[b]) continue;

        uint8_t *block = fs->image + dir->i_block[b] * fs->block_size;
        uint32_t offset = 0;

        while (offset < fs->block_size) 
        {
            ext2_dir_entry_t *de = (ext2_dir_entry_t*)(block + offset);

            if (de->inode) 
            {
                char name[256] = {0};
                kpmemcpy(name, de->name, de->name_len);
                if (!cb(name, de->inode, ctx)) return;
            }

            if (de->rec_len == 0) break;
            
            offset += de->rec_len;
        }
    }
}

size_t ext2_read_file(ext2_fs_t *fs, ext2_inode_t *inode,
                      void *out_buf, size_t buf_len, size_t offset)
{
    if (!inode || !out_buf) return 0;
    
    uint32_t file_size = inode->i_size_lo;
    if (offset >= file_size) return 0;
    
    // Limit read to file size
    if (offset + buf_len > file_size) 
    {
        buf_len = file_size - offset;
    }
    
    size_t bytes_read = 0;
    uint8_t *output = (uint8_t*)out_buf;
    
    // For simplicity, only handle direct blocks (first 12 blocks)
    uint32_t blocks_to_read = (offset + buf_len + fs->block_size - 1) / fs->block_size;
    uint32_t start_block = offset / fs->block_size;
    uint32_t start_offset = offset % fs->block_size;
    
    for (uint32_t b = start_block; b < 12 && b < blocks_to_read && bytes_read < buf_len; b++) 
    {
        if (!inode->i_block[b]) 
        {
            // Sparse block - fill with zeros
            size_t bytes_in_block = fs->block_size;

            if (b == start_block) bytes_in_block -= start_offset;

            if (bytes_read + bytes_in_block > buf_len) 
            {
                bytes_in_block = buf_len - bytes_read;
            }
            
            kpmemset(output + bytes_read, 0, bytes_in_block);
            bytes_read += bytes_in_block;
            continue;
        }
        
        uint8_t *block = fs->image + inode->i_block[b] * fs->block_size;
        size_t copy_offset = (b == start_block) ? start_offset : 0;
        size_t bytes_in_block = fs->block_size - copy_offset;
        
        if (bytes_read + bytes_in_block > buf_len) 
        {

            bytes_in_block = buf_len - bytes_read;
        }
        
        kpmemcpy(output + bytes_read, block + copy_offset, bytes_in_block);
        bytes_read += bytes_in_block;
    }
    
    return bytes_read;
}