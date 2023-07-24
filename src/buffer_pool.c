#include "buffer_pool.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

//long long test_cnt = 0;

void init_buffer_pool(const char *filename, BufferPool *pool) {
    FileIOResult fior = open_file(&(pool->file),filename);
    if(fior != FILE_IO_SUCCESS){
        printf("file io fail: result = %d",fior);
        return;
    } 
    for(int i=0;i<CACHE_PAGE;++i){
        pool->addrs[i] = -1;
        pool->cnt[i] = 0;
        pool->ref[i] = 0;
    }
}

void close_buffer_pool(BufferPool *pool) {
    for(int i=0;i<CACHE_PAGE;++i){
        if(pool->addrs[i] == -1) continue;
        FileIOResult fior = write_page(&(pool->pages[i]),&(pool->file),pool->addrs[i]);
        if(fior != FILE_IO_SUCCESS){
            printf("file io fail: result = %d",fior);
            return;
        } 
    }
    close_file(&(pool->file));
    //free(pool);
}

Page *get_page(BufferPool *pool, off_t addr) {//LRU
    //test_cnt++;
    //if(test_cnt % 10000000 == 0) printf("%lld\n",test_cnt); 
    int pos=CACHE_PAGE;
    size_t maxcnt=0;
    for(int i=0;i<CACHE_PAGE;++i){
        ++pool->cnt[i];
        if(pool->addrs[i] == addr){
            pos = i;
        } 
    }
    if(pos != CACHE_PAGE){
        pool->cnt[pos] = 0;
        ++pool->ref[pos];
        return &(pool->pages[pos]);
    }
    else{
        for(int j=0;j<CACHE_PAGE;++j){
            if(maxcnt < pool->cnt[j] && pool->ref[j] == 0){
                maxcnt = pool->cnt[j];
                pos = j;
            }
        }
        FileIOResult fior;
        if(pool->addrs[pos] != -1){
            fior = write_page(&(pool->pages[pos]),&(pool->file),pool->addrs[pos]);
            if(fior != FILE_IO_SUCCESS){
                printf("file io fail: result = %d",fior);
                return NULL;
            }
        }
        fior = read_page(&(pool->pages[pos]),&(pool->file),addr);
        pool->addrs[pos] = addr;
        pool->cnt[pos] = 0;
        pool->ref[pos] = 1;
        return &(pool->pages[pos]);
    }
}

void release(BufferPool *pool, off_t addr) {
    for(int i=0;i<CACHE_PAGE;++i){
        if(pool->addrs[i] == addr){
            --pool->ref[i];
            return;
        }
    }
    printf("release not found addr! %lld",addr);
}

/* void print_buffer_pool(BufferPool *pool) {
} */

/* void validate_buffer_pool(BufferPool *pool) {
} */
