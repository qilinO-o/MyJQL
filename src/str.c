#include "str.h"

#include "table.h"
#include "string.h"

void read_string(Table *table, RID rid, StringRecord *record) {
    record->idx = 0;
    StringChunk *chunk = &(record->chunk);
    table_read(table,rid,(ItemPtr)chunk);
}

int has_next_char(StringRecord *record) {
    // RID rid_NULL;
    // get_rid_block_addr(rid_NULL) = (off_t)(-1);
    // get_rid_idx(rid_NULL) = (short)(0);
    StringChunk *chunk = &(record->chunk);
    if(record->idx >= get_str_chunk_size(chunk)){
        RID chunk_rid = get_str_chunk_rid(chunk);
        if(get_rid_block_addr(chunk_rid) == (off_t)(-1)){
            return 0;
        }
    }
    return 1;
}

char next_char(Table *table, StringRecord *record) {
    StringChunk *chunk = &(record->chunk);
    char res_c;
    if(record->idx >= get_str_chunk_size(chunk)){
        RID next_chunk_rid = get_str_chunk_rid(chunk);
        read_string(table, next_chunk_rid, record);
        res_c = get_str_chunk_data_ptr(chunk)[record->idx];
        ++record->idx;
    }
    else{
        res_c = get_str_chunk_data_ptr(chunk)[record->idx];
        ++record->idx;
    }
    return res_c;
}

//if a > b then return 1, a < b return -1, a = b return 0
int compare_string_record(Table *table, const StringRecord *a, const StringRecord *b) {
    StringRecord sra = *a;
    StringRecord srb = *b;
    while(1){
        int has_nextchar_a = has_next_char(&sra);
        int has_nextchar_b = has_next_char(&srb);
        if(has_nextchar_a && has_nextchar_b){
            char a_c = next_char(table,&sra);
            char b_c = next_char(table,&srb);
            if(a_c > b_c) return 1;
            else if(a_c < b_c) return -1;
            else continue;
        }
        else if(has_nextchar_a && !has_nextchar_b){
            return 1;
        }
        else if(!has_nextchar_a && has_nextchar_b){
            return -1;
        }
        else return 0;
    }
}

int compare_string_string_record(Table *table, char *a, size_t size, const StringRecord *b) {
    int i = 0;
    StringRecord srb = *b;
    while(1){
        int has_nextchar_a = (i < size) ? 1 : 0;
        int has_nextchar_b = has_next_char(&srb);
        if(has_nextchar_a && has_nextchar_b){
            char a_c = a[i++];
            char b_c = next_char(table,&srb);
            if(a_c > b_c) return 1;
            else if(a_c < b_c) return -1;
            else continue;
        }
        else if(has_nextchar_a && !has_nextchar_b){
            return 1;
        }
        else if(!has_nextchar_a && has_nextchar_b){
            return -1;
        }
        else return 0;
    }
}



RID write_string(Table *table, const char *data, off_t size) {
    RID pre_rid;
    get_rid_block_addr(pre_rid) = (off_t)(-1);
    get_rid_idx(pre_rid) = (short)(0);
    if(size == 0){
        StringChunk newchunk;
        get_str_chunk_rid(&newchunk) = pre_rid;
        get_str_chunk_size(&newchunk) = 0;
        //memmove((get_str_chunk_data_ptr(&newchunk)),data + (size - rest),rest);
        pre_rid = table_insert(table,(ItemPtr)&newchunk, calc_str_chunk_size(0));
    }
    int rest = size % STR_CHUNK_MAX_LEN;
    if(rest != 0){
        StringChunk newchunk;
        get_str_chunk_rid(&newchunk) = pre_rid;
        get_str_chunk_size(&newchunk) = rest;
        memmove((get_str_chunk_data_ptr(&newchunk)),data + (size - rest),rest);
        pre_rid = table_insert(table,(ItemPtr)&newchunk, (short)calc_str_chunk_size(rest));
        size -= rest;
    }
    while(size!=0){
        StringChunk newchunk;
        get_str_chunk_rid(&newchunk) = pre_rid;
        get_str_chunk_size(&newchunk) = STR_CHUNK_MAX_LEN;
        memmove((get_str_chunk_data_ptr(&newchunk)),data + (size - STR_CHUNK_MAX_LEN),STR_CHUNK_MAX_LEN);
        pre_rid = table_insert(table,(ItemPtr)&newchunk,(short)sizeof(newchunk));
        size -= STR_CHUNK_MAX_LEN;
    }
    return pre_rid;
}

void delete_string(Table *table, RID rid) {
    RID next_chunk_rid = rid;
    while(get_rid_block_addr(next_chunk_rid) != -1){
        StringChunk chunk;
        table_read(table, next_chunk_rid, (ItemPtr)&chunk);
        table_delete(table,next_chunk_rid);
        next_chunk_rid = get_str_chunk_rid(&chunk);
    }
}

void print_string(Table *table, const StringRecord *record) {
    StringRecord rec = *record;
    printf("\"");
    while (has_next_char(&rec)) {
        printf("%c", next_char(table, &rec));
    }
    printf("\"");
}

size_t load_string(Table *table, const StringRecord *record, char *dest, size_t max_size) {
    StringRecord str_record = *record;
    // int start = 0;
    size_t res_size = 0;
    // while(1){
    //     size_t this_size = get_str_chunk_size(&(str_record.chunk));
    //      if(res_size + this_size > max_size){
    //         memmove(dest + start, get_str_chunk_data_ptr(&(str_record.chunk)), max_size - res_size);
    //         res_size = max_size;
    //         break;
    //     }
    //     memmove(dest + start, get_str_chunk_data_ptr(&(str_record.chunk)), this_size);
       
    //     res_size +=this_size;
    //     RID next_chunk_rid = get_str_chunk_rid(&(str_record.chunk));
    //     if(get_rid_block_addr(next_chunk_rid) == -1){
    //         break;
    //     }
    //     else{
    //         table_read(table, next_chunk_rid, (ItemPtr)&(str_record.chunk));
    //     }
    // }
    while (has_next_char(&str_record) && res_size < max_size) {
        dest[res_size++] = next_char(table, &str_record);
    }
    return res_size;
}

/* void chunk_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    StringChunk *chunk = (StringChunk*)item;
    short size = get_str_chunk_size(chunk), i;
    printf("StringChunk(");
    print_rid(get_str_chunk_rid(chunk));
    printf(", %d, \"", size);
    for (i = 0; i < size; i++) {
        printf("%c", get_str_chunk_data_ptr(chunk)[i]);
    }
    printf("\")");
} */