#ifndef _MEM_ALLOC_H
#define _MEM_ALLOC_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define BYTE_ARRAY  1
#define U16_ARRAY   2
#define U32_ARRAY   3
#define FLOAT_ARRAY 4

#define UNUSD   1
#define USED    2
#define FREED   3

#define LARGEMEM_COUNT  0x10
#define PAGE_COUNT 0x400
#define MAX_FREE_COUNT 0x10

#define SIZE2IDX(size) ((size>>2) - 1)
#define MAX_ALLOC_SIZE (128*1024*1024)
#define CHUNK_HEAD_SIZE 0x4

struct Chunk{
    struct Page* page_addr;     // low 12 bit used to represent chunk's size
    struct Chunk* next;     // only used in free
};

struct Page{
    uint32_t chunk_size;
    uint32_t chunk_count;
    uint8_t* chunk_addr;
    uint32_t free_chunk_count;
    struct Chunk* free_list;
    struct Page* next_page;
    struct Page* prev_page;
    struct Mem_manager* memManager;
    uint8_t chunk_status[4];
};

// struct Userheap{
//     uint32_t length;
//     uint32_t size;
//     void* addr;
// };

struct Mem_manager{
    uint32_t size_threshold;        // If the size allocated by the user exceeds the threshold, directly using mmap
    void* large_mem[LARGEMEM_COUNT];
    uint32_t large_size[LARGEMEM_COUNT];
    uint8_t  large_mem_status[LARGEMEM_COUNT];
    struct Page* pages[PAGE_COUNT];
    uint8_t page_status[PAGE_COUNT];
};

struct Random{
    uint8_t randBytes[0x1008];
    uint32_t idx;
    uint32_t size;
};

struct Array{
    // uint32_t type;
    uint32_t element_size;
    uint32_t length;
    uint8_t* element_addr;
};


void error_exit(const char* msg);

void* mem_alloc(uint32_t size);
void mem_free(void* addr);

#endif