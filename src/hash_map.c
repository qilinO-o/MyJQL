#include "hash_map.h"

#include <stdio.h>
#include <string.h>

void hash_table_init(const char *filename, BufferPool *pool, off_t n_directory_blocks) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    if(pool->file.length != 0) return;
    Page* ctrblock_page = get_page(pool,0);
    HashMapControlBlock* ctrblock = (HashMapControlBlock*) ctrblock_page;
    ctrblock->n_directory_blocks = n_directory_blocks;
    ctrblock->free_block_head = PAGE_SIZE * (n_directory_blocks + 1);
    ctrblock->max_size = 8;
    write_page(ctrblock_page, &pool->file, 0);
    release(pool,0);
    for(int i=0;i<n_directory_blocks;++i){
        HashMapDirectoryBlock dirblock;
        memset(dirblock.directory,(off_t)-1,sizeof(dirblock.directory));
        write_page((const Page*)&dirblock,&pool->file,PAGE_SIZE*(i+1));
    }
    for(int i=0;i<ctrblock->max_size;++i){
        HashMapBlock mapblock;
        if(i != ctrblock->max_size-1){
            mapblock.n_items = 0;
            mapblock.next = ctrblock->free_block_head + PAGE_SIZE*(i+1);
        }
        else{
            mapblock.n_items = 0;
            mapblock.next = -1;
        }
        write_page((const Page*)&mapblock,&pool->file,ctrblock->free_block_head + PAGE_SIZE*i);
    }
}

void hash_table_close(BufferPool *pool) {
    close_buffer_pool(pool);
}

void hash_table_insert(BufferPool *pool, short size, off_t addr) {
    if(size<0 || size>=128) {
        //printf("invalid size when insert\n");
        return;
    }
    HashMapControlBlock* ctrblock_ptr = (HashMapControlBlock*)get_page(pool,0);
    Page* target_page = get_page(pool,PAGE_SIZE*((size/HASH_MAP_DIR_BLOCK_SIZE) + 1));
    off_t* ptr = &(((HashMapDirectoryBlock*)target_page)->directory[size%HASH_MAP_DIR_BLOCK_SIZE]);
    if(*ptr == -1){
        off_t blank_mapblock_ptr = ctrblock_ptr->free_block_head;
        if(blank_mapblock_ptr != -1){
            HashMapBlock* blank_mapblock = (HashMapBlock*)get_page(pool,blank_mapblock_ptr);
            ctrblock_ptr->free_block_head = blank_mapblock->next;
            blank_mapblock->n_items = 1;
            blank_mapblock->next = -1;
            blank_mapblock->table[0] = addr;
            release(pool,blank_mapblock_ptr);
            *ptr = blank_mapblock_ptr;
        }
        else{
            HashMapBlock newmapblock;
            newmapblock.n_items = 1;
            newmapblock.next = -1;
            newmapblock.table[0] = addr;
            write_page((const Page*)&newmapblock,&pool->file, + PAGE_SIZE*(1+ctrblock_ptr->n_directory_blocks+ctrblock_ptr->max_size));
            *ptr = PAGE_SIZE*(1+ctrblock_ptr->n_directory_blocks+ctrblock_ptr->max_size);
            ++ctrblock_ptr->max_size;
        }
    }
    else{
        off_t tempptr = *ptr;
        while(tempptr != -1){
            HashMapBlock* mapblock = (HashMapBlock*)get_page(pool,tempptr);
            if(mapblock->n_items >= HASH_MAP_BLOCK_SIZE){
                off_t pre = tempptr;
                tempptr = mapblock->next;
                release(pool,pre);
            } 
            else{
                release(pool,tempptr);
                break;
            } 
        }
        if(tempptr != -1){
            HashMapBlock* mapblock = (HashMapBlock*)get_page(pool,tempptr);
            mapblock->table[mapblock->n_items] = addr;
            ++mapblock->n_items;
            release(pool,tempptr);
        }
        else{
            HashMapBlock newmapblock;
            newmapblock.n_items = 1;
            newmapblock.next = *ptr;
            newmapblock.table[0] = addr;
            write_page((const Page*)&newmapblock,&pool->file,PAGE_SIZE*(1+ctrblock_ptr->n_directory_blocks+ctrblock_ptr->max_size));
            *ptr = PAGE_SIZE*(1+ctrblock_ptr->n_directory_blocks+ctrblock_ptr->max_size);
            ++ctrblock_ptr->max_size;
        }
    }
    release(pool,0);
    release(pool,PAGE_SIZE*((size/HASH_MAP_DIR_BLOCK_SIZE) + 1));
}

off_t hash_table_pop_lower_bound(BufferPool *pool, short size) {
    if(size<0 || size>=128) {
        //printf("invalid size when pop lower bound\n");
        return -1;
    }
    HashMapControlBlock* ctrblock_ptr = (HashMapControlBlock*)get_page(pool,0);
    short target_size = size;
    off_t start_offset = PAGE_SIZE*((size/HASH_MAP_DIR_BLOCK_SIZE) + 1);
    for(int i=0;i<ctrblock_ptr->n_directory_blocks;++i){
        HashMapDirectoryBlock* dirblock_ptr = (HashMapDirectoryBlock*)get_page(pool,start_offset+i*PAGE_SIZE);
        int j;
        if(i == 0) j = size%HASH_MAP_DIR_BLOCK_SIZE;
        else j = 0;
        off_t ptr;
        for(;j<HASH_MAP_DIR_BLOCK_SIZE;++j){
            ptr = dirblock_ptr->directory[j];
            if(ptr != -1){
                HashMapBlock* mapblock = (HashMapBlock*)get_page(pool,ptr);
                off_t res = mapblock->table[mapblock->n_items-1];
                release(pool,ptr);
                release(pool,start_offset+i*PAGE_SIZE);
                release(pool,0);
                hash_table_pop(pool,target_size,res);
                return res;
            }
            ++target_size;
            if (target_size >= 128) {
                release(pool, start_offset + i * PAGE_SIZE);
                release(pool, 0);
                return -1;
            }
        }
        release(pool,start_offset+i*PAGE_SIZE);
    }
    release(pool,0);
    return -1;
}

void hash_table_pop(BufferPool *pool, short size, off_t addr) {
    if(size<0 || size>=128) {
        //printf("invalid size when pop\n");
        return;
    }
    HashMapControlBlock* ctrblock_ptr = (HashMapControlBlock*)get_page(pool,0);
    HashMapDirectoryBlock* target_dirblock_ptr = (HashMapDirectoryBlock*)get_page(pool,PAGE_SIZE*((size/HASH_MAP_DIR_BLOCK_SIZE) + 1));
    off_t* offset_ptr = &(target_dirblock_ptr->directory[size%HASH_MAP_DIR_BLOCK_SIZE]);
    off_t* i = offset_ptr;
    while(*i != -1){
        HashMapBlock* mapblock = (HashMapBlock*)get_page(pool,*i);
        short if_pop_success = 0;
        for(int j=0;j<mapblock->n_items;++j){
            if(mapblock->table[j] == addr){
                if(mapblock->n_items == 1){
                    off_t temp_next = mapblock->next;
                    mapblock->next = ctrblock_ptr->free_block_head;
                    mapblock->n_items = 0;
                    ctrblock_ptr->free_block_head = *i;
                    release(pool, *i);
                    *i = temp_next;
                }
                else{
                    if(mapblock->n_items-1-j !=0 ) memmove(mapblock->table+j,mapblock->table+j+1,(mapblock->n_items-1-j)*sizeof(off_t));
                    --mapblock->n_items;
                    release(pool, *i);
                }
                if_pop_success = 1;
                break;
            }
        }
        if (if_pop_success) break;
        else{
            release(pool, *i);
        }
        i = &(mapblock->next);
    }
    release(pool,PAGE_SIZE*((size/HASH_MAP_DIR_BLOCK_SIZE) + 1));
    release(pool,0);
    return;
}

 void print_hash_table(BufferPool *pool) {
    HashMapControlBlock *ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock *dir_block;
    off_t block_addr, next_addr;
    HashMapBlock *block;
    int i, j;
    printf("----------HASH TABLE----------\n");
    for (i = 0; i < ctrl->n_directory_blocks; ++i) {
        dir_block = (HashMapDirectoryBlock*)get_page(pool, (i + 1) * PAGE_SIZE);
        for(int k=0;k<HASH_MAP_DIR_BLOCK_SIZE;++k){
            if (dir_block->directory[k % HASH_MAP_DIR_BLOCK_SIZE] != -1) {
                printf("%d %d:", i,i*HASH_MAP_DIR_BLOCK_SIZE+k);
                block_addr = dir_block->directory[k % HASH_MAP_DIR_BLOCK_SIZE];
                while (block_addr != -1) {
                    block = (HashMapBlock*)get_page(pool, block_addr);
                    printf("  [" FORMAT_OFF_T "]", block_addr);
                    printf("{");
                    for (j = 0; j < block->n_items; ++j) {
                        if (j != 0) {
                            printf(", ");
                        }
                        printf(FORMAT_OFF_T, block->table[j]);
                    }
                    printf("}");
                    next_addr = block->next;
                    release(pool, block_addr);
                    block_addr = next_addr;
                }
                printf("\n");
            }
        }
        
        release(pool, (i + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    printf("------------------------------\n");
} 