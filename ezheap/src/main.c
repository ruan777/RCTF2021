#include "mem_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define HeapCount 0x400

struct Array* ByteArrays[HeapCount];
struct Array* U16Arrays[HeapCount];
struct Array* U32Arrays[HeapCount];
struct Array* FloatArrays[HeapCount];


void handler(int sig){
	puts("time out!");
	_exit(2);
}

void init_iobuf(void){
    setvbuf(stdin,NULL,_IONBF,0);
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
    signal(SIGALRM, handler);
	alarm(0x200);
}

void banner(){
    puts(
    "  _____   _____ _______ ______   ___   ___ ___  __ \n"
    " |  __ \\ / ____|__   __|  ____| |__ \\ / _ \\__ \\/_ |\n"
    " | |__) | |       | |  | |__       ) | | | | ) || |\n"
    " |  _  /| |       | |  |  __|     / /| | | |/ / | |\n"
    " | | \\ \\| |____   | |  | |       / /_| |_| / /_ | |\n"
    " |_|  \\_\\\\_____|  |_|  |_|      |____|\\___/____||_|\n"
    );
}

void init(){
    init_iobuf();
    banner();
}

void menu(){
    puts("1. alloc");
    puts("2. edit");
    puts("3. view");
    puts("4. delete");
    puts("5. exit");
    puts("enter your choice>>");
}

void type_menu(){
    puts("1. ByteArray");
    puts("2. Uint16Array");
    puts("3. Uint32Array");
    puts("4. FloatArray");
    puts("which type >>");
}

uint32_t read_n(char* buf,uint32_t len){
    int i;
    for(i = 0;i < len;i++){
        read(0,buf+i,1);
        if(buf[i] == '\n'){
            break;
        }
    }
    return i;
}

uint32_t get_int(){
    char buf[0x10] = {0};
    read_n(buf,0xf);
    return (uint32_t)atoll(buf);
}

double get_float(){
    char buf[0x30] = {0};
    double res;
    read_n(buf,0x2f);
    sscanf(buf,"%lf",&res);
    return res;
}

void alloc_Array(uint32_t type){
    uint32_t size;
    uint32_t align_size;
    uint32_t idx;
    void* ptr;


    puts("size>>");
    size = get_int();
    puts("idx>>");
    idx = get_int();
    
    if(idx > HeapCount){
        puts("invalid idx");
        return;
    }

    switch(type)
    {
        case BYTE_ARRAY:
            // no need align
            ptr = mem_alloc(size);
            if(ptr == NULL){
                puts("mem alloc error!");
                return;
            }
            ByteArrays[idx] = mem_alloc(sizeof(struct Array));
            ByteArrays[idx]->element_addr = ptr;
            ByteArrays[idx]->element_size = sizeof(uint8_t);
            ByteArrays[idx]->length = size / ByteArrays[idx]->element_size;
            // ByteArrays[idx]->type = BYTE_ARRAY;
            break;
        case U16_ARRAY:
            align_size = ((size + 1u) >> 1u) << 1u;      // align to 2
            ptr = mem_alloc(size);
            if(ptr == NULL){
                puts("mem alloc error!");
                return;
            }
            U16Arrays[idx] = mem_alloc(sizeof(struct Array));
            U16Arrays[idx]->element_addr = ptr;
            U16Arrays[idx]->element_size = sizeof(uint16_t);
            U16Arrays[idx]->length = align_size / U16Arrays[idx]->element_size;
            // U16Arrays[idx]->type = U16_ARRAY;
            break;
        case U32_ARRAY:
            align_size = ((size + 3u) >> 2u) << 2u;      // align to 4
            ptr = mem_alloc(size);
            if(ptr == NULL){
                puts("mem alloc error!");
                return;
            }
            U32Arrays[idx] = mem_alloc(sizeof(struct Array));
            U32Arrays[idx]->element_addr = ptr;
            U32Arrays[idx]->element_size = sizeof(uint32_t);
            U32Arrays[idx]->length = align_size / U32Arrays[idx]->element_size;
            // U32Arrays[idx]->type = U32_ARRAY;
            break;
        case FLOAT_ARRAY:
            align_size = ((size + 7u) >> 3u) << 3u;      // align to 8
            ptr = mem_alloc(size);
            if(ptr == NULL){
                puts("mem alloc error!");
                return;
            }
            FloatArrays[idx] = mem_alloc(sizeof(struct Array));
            FloatArrays[idx]->element_addr = ptr;
            FloatArrays[idx]->element_size = sizeof(double);
            FloatArrays[idx]->length = align_size / FloatArrays[idx]->element_size;
            // FloatArrays[idx]->type = FLOAT_ARRAY;
            break;
        default:
            puts("invalid type!");
            return;
    }
}

void edit_Array(uint32_t type){
    uint32_t idx;
    uint32_t element_idx;
    uint32_t value = 0;
    double float_value = 0.0;

    puts("idx>>");
    idx = get_int();
    switch(type)
    {
        case BYTE_ARRAY:
            if(ByteArrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > ByteArrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            ByteArrays[idx]->element_addr[element_idx] = get_int() & 0xff;
            break;
        case U16_ARRAY:
            if(U16Arrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > U16Arrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            *(uint16_t*)(U16Arrays[idx]->element_addr + element_idx * 2) = get_int() & 0xffff;
            break;
        case U32_ARRAY:
            if(U32Arrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > U32Arrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            *(uint32_t*)(U32Arrays[idx]->element_addr + element_idx * 4) = get_int();
            break;
        case FLOAT_ARRAY:
            if(FloatArrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > FloatArrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            *(double*)(&(FloatArrays[idx]->element_addr[element_idx * 8])) = get_float();
            break;
        default:
            puts("invalid type!");
            return; 
    }
}

void view_Array(uint32_t type){
    uint32_t idx;
    uint32_t element_idx;

    puts("idx>>");
    idx = get_int();
    switch(type)
    {
        case BYTE_ARRAY:
            if(ByteArrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > ByteArrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            printf("%u\n",ByteArrays[idx]->element_addr[element_idx]&0xff);
            break;
        case U16_ARRAY:
            if(U16Arrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > U16Arrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            printf("%u\n",*(uint16_t*)(U16Arrays[idx]->element_addr + element_idx * 2));
            break;
        case U32_ARRAY:
            if(U32Arrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > U32Arrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            printf("%u\n",*(uint32_t*)(U32Arrays[idx]->element_addr + element_idx * 4));
            break;
        case FLOAT_ARRAY:
            if(FloatArrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            puts("element_idx>>");
            element_idx = get_int();
            if(element_idx + 1 > FloatArrays[idx]->length){
                puts("invalid element_idx!");
                return;
            }
            puts("value>>");
            printf("%f\n",*(double*)(&(FloatArrays[idx]->element_addr[element_idx * 8])));
            break;
        default:
            puts("invalid type!");
            return; 
    }
}

void delete_Array(uint32_t type){
    uint32_t idx;

    puts("idx>>");
    idx = get_int();
    switch(type)
    {
        case BYTE_ARRAY:
            if(ByteArrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            mem_free(ByteArrays[idx]->element_addr);
            memset(ByteArrays[idx],0,sizeof(struct Array));
            mem_free(ByteArrays[idx]);
            ByteArrays[idx] = NULL;
            break;
        case U16_ARRAY:
            if(U16Arrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            mem_free(U16Arrays[idx]->element_addr);
            memset(U16Arrays[idx],0,sizeof(struct Array));
            mem_free(U16Arrays[idx]);
            U16Arrays[idx] = NULL;
            break;
        case U32_ARRAY:
            if(U32Arrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            mem_free(U32Arrays[idx]->element_addr);
            memset(U32Arrays[idx],0,sizeof(struct Array));
            mem_free(U32Arrays[idx]);
            U32Arrays[idx] = NULL;
            break;
        case FLOAT_ARRAY:
            if(FloatArrays[idx] == NULL){
                puts("invalid idx!");
                return;
            }
            mem_free(FloatArrays[idx]->element_addr);
            memset(FloatArrays[idx],0,sizeof(struct Array));
            mem_free(FloatArrays[idx]);
            FloatArrays[idx] = NULL;
            break;
        default:
            puts("invalid type!");
            return; 
    }
}

void alloc(){
    uint32_t type = 0;
    type_menu();
    type = get_int();
    alloc_Array(type);
}

void edit(){
    uint32_t type = 0;
    type_menu();
    type = get_int();
    edit_Array(type);
}

void view(){
    uint32_t type = 0;
    type_menu();
    type = get_int();
    view_Array(type);
}

void delete(){
    uint32_t type = 0;
    type_menu();
    type = get_int();
    delete_Array(type);
}

int main(){
    init();
    int choice = 0;

    while(choice != 5){
        menu();
        choice = get_int();
        switch (choice)
        {
        case 1:
            alloc();
            break;
        case 2:
            edit();
            break;
        case 3:
            view();
            break;
        case 4:
            delete();
            break;
        case 5:
            puts("good bye!");
            break;
        }
    }
    return 0;
}