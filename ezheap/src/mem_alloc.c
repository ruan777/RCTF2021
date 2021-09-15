#include "mem_alloc.h"
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

// #define DEBUG

struct Mem_manager manager;
struct Random r;
uint32_t is_initial = 0;

void error_exit(const char* msg){
    puts(msg);
    exit(-1);
}

void init_manager(){
    memset(&manager,sizeof(manager),0);
    memset(&(manager.large_mem_status),FREED,sizeof(uint8_t)*LARGEMEM_COUNT);
    memset(&(manager.page_status),UNUSD,sizeof(uint8_t)*PAGE_COUNT);
    manager.size_threshold = 0x1000;
    is_initial = 1;
}

void init_random(){
    int fd = open("/dev/urandom",O_RDONLY);
    if(fd < 0){
        error_exit("open urandom failed! what happen??");
    }
    r.size = 0x1000;
    r.idx = 0;
    read(fd,r.randBytes,r.size);
    close(fd);
}

void* large_mem_alloc(uint32_t size){
    int idx = 0;
    for(idx = 0; idx < LARGEMEM_COUNT;idx++){
        if(manager.large_mem_status[idx] == FREED)
            break;
    }
    if(idx == LARGEMEM_COUNT){
        puts("no large mem available");
        return NULL;
    }
    void* res = (struct Page*)mmap(NULL,size,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANON,-1,0);
    if(res == MAP_FAILED){
        error_exit("mmap failed!");
    }
    manager.large_mem_status[idx] = USED;
    manager.large_size[idx] = size;
    manager.large_mem[idx] = res;
#ifdef DEBUG
    printf("alloc size: 0x%x, ret addr: %p\n",size,res);
#endif
    return res;
}


uint32_t randU32(){
    uint32_t res;
    if(r.idx + 4 > r.size){
        init_random(); 
    }
    res = *(uint32_t*)(&r.randBytes[r.idx]);
    r.idx += 4;
    return res;
}

uint8_t randU8(){
    uint8_t res;
    if(r.idx + 1 > r.size){
        init_random(); 
    }
    res = *(uint8_t*)(&r.randBytes[r.idx]);
    r.idx += 4;
    return res;
}

struct Page* init_page(uint32_t chunk_size){
    struct Page* page_addr = (struct Page*)mmap(NULL,0x1000,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANON,-1,0);
    if(page_addr == MAP_FAILED){
        error_exit("mmap failed!");
    }
    page_addr->chunk_addr = (uint8_t*)mmap(NULL,0x4000,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANON,-1,0);
    if(page_addr->chunk_addr == MAP_FAILED){
        error_exit("mmap failed!");
    }
    page_addr->chunk_size = chunk_size + CHUNK_HEAD_SIZE;
    page_addr->chunk_count = 0x4000 / page_addr->chunk_size;
    page_addr->free_chunk_count = 0;
    page_addr->free_list = NULL;
    page_addr->next_page = NULL;
    page_addr->prev_page = NULL;
    page_addr->memManager = &manager;
    memset(page_addr->chunk_status,FREED,page_addr->chunk_count);
    return page_addr;
}

struct Page* get_page(uint32_t idx){
    return manager.pages[idx];
}

void* mem_alloc(uint32_t size){
    void* res;
    if(!is_initial){
        init_manager();
        init_random();
    }
    if(size <= 0 || size > MAX_ALLOC_SIZE){
        puts("invalid size!");
        return NULL;
    }
    uint32_t align_size = ((size + 3u) >> 2u) << 2u;
    if(align_size + CHUNK_HEAD_SIZE >= manager.size_threshold){
        // use large mem alloc
        res = large_mem_alloc(size);
        return res;
    }
    int idx = SIZE2IDX(align_size);
    if(manager.page_status[idx] == UNUSD){
        manager.pages[idx] = init_page(align_size);
        manager.page_status[idx] = USED;
    }
    struct Page* page = get_page(idx);
    struct Page* iter_page = page;

    // first check free_list
    while(iter_page != NULL){
        if(iter_page->free_chunk_count > 0){
        // remove one chunk from free_list's head
        res = iter_page->free_list;
        struct Chunk* tmp = ((struct Chunk*)res)->next;
        iter_page->free_list = tmp;
        iter_page->free_chunk_count--;
#ifdef DEBUG
    printf("free_list--alloc size: 0x%x, ret addr: %p\n",align_size + CHUNK_HEAD_SIZE,res + CHUNK_HEAD_SIZE);
#endif
        return res + CHUNK_HEAD_SIZE;
        }
        iter_page = iter_page->next_page;
    }
    

    uint32_t random_idx = randU32() % page->chunk_count;
    uint32_t collision_count = 0;

    if(page->chunk_status[random_idx] == FREED){
        res = page->chunk_addr + page->chunk_size * random_idx;
        page->chunk_status[random_idx] = USED;
    }else{
        // collision
        do{
            random_idx = randU32() % page->chunk_count;
            if(page->chunk_status[random_idx] == FREED){
                res = page->chunk_addr + page->chunk_size * random_idx;
                page->chunk_status[random_idx] = USED;
                break;
            }
            collision_count++;
        }while(collision_count < 3);
        // to many collision, we need to alloc new page
        if(collision_count == 3){
            // printf("collision\n");
            struct Page* next = init_page(align_size);
            next->next_page = page;
            page->prev_page = next;
            manager.pages[idx] = next;
            page = next;
            // this is a new alloced page, so we don't need check
            random_idx = randU32() % page->chunk_count;
            res = page->chunk_addr + page->chunk_size * random_idx;
            page->chunk_status[random_idx] = USED;
        }
    }
    ((struct Chunk*)res)->page_addr = (struct Page*)((uint32_t)page | (align_size + CHUNK_HEAD_SIZE));
#ifdef DEBUG
    printf("random_idx : 0x%x, alloc size: 0x%x, ret addr: %p\n",random_idx,align_size + CHUNK_HEAD_SIZE,res + CHUNK_HEAD_SIZE);
#endif
    return res + CHUNK_HEAD_SIZE;
}

void mem_free(void* addr){
    if(addr == NULL)
        return;
    // check addr is a large mem or not
    int idx = 0;
    for(idx = 0;idx < LARGEMEM_COUNT;idx++){
        if(manager.large_mem_status[idx] == USED && manager.large_mem[idx] == addr){
            munmap(addr,manager.large_size[idx]);
            manager.large_mem_status[idx] = FREED;
#ifdef DEBUG
    printf("freed %p\n",addr);
#endif
            return;
        }
    }

    struct Chunk* chunk = (struct Chunk*)((uint8_t*)addr - CHUNK_HEAD_SIZE);
    struct Page* page = (struct Page*)((uint32_t)chunk->page_addr & 0xfffff000);
    uint32_t chunk_size = (uint32_t)chunk->page_addr & 0xfff;
    if(chunk_size != page->chunk_size){
        error_exit("invalid size");
    }
    // if free_list_count does not reach the limit
    if(page->free_chunk_count < MAX_FREE_COUNT){
        // insert chunk into free_list's head
        void* tmp = page->free_list;
        page->free_list = chunk;
        chunk->next = tmp;
        page->free_chunk_count++;
#ifdef DEBUG
    printf("freed %p\n",chunk);
#endif
        return;
    }

    uint8_t* chunk_start_addr = page->chunk_addr;
    uint32_t chunk_idx = (uint32_t)((uint8_t*)chunk - chunk_start_addr) / page->chunk_size;
    page->chunk_status[chunk_idx] = FREED;
#ifdef DEBUG
    printf("freed %p\n",chunk);
#endif
}