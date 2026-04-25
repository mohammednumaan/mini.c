#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../include/chunk.h"

/*
 *
 *
 * learn about alignmetn and how to do them in c
*/

static Chunk *heap_start = NULL;
static void *heap_end = NULL;

void heap_init(void){
    void *memory = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(memory != MAP_FAILED);

    heap_start = (Chunk *)memory;
    heap_start->size = HEAP_SIZE - CHUNK_HEADER_SIZE;
    heap_start->is_free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;
    heap_end = (char *)memory + HEAP_SIZE;
}

Chunk* ch_find_free_chunk(size_t size){
    // start from heap_start and traverse all the chunks
    // need to choose an allocation strategy, for now i'm going with 'first-fit'
    // because its very easy to implement
    Chunk *curr = heap_start;
    while (curr){
        // this is first-fit, we return the first chunk 
        // that is free and has enough size
        if (curr->is_free && curr->size >= size){
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

void ch_split_chunk(Chunk *allocated_chunk, size_t size){
    if (allocated_chunk->size < size + CHUNK_HEADER_SIZE) return;

    // this is the memory available after "size" bytes
    // [HEADER][....size....][-----free-space-----]
    Chunk *new_chunk = (Chunk *)((char *)(allocated_chunk + 1) + size);
    new_chunk->size = allocated_chunk->size - size -CHUNK_HEADER_SIZE;
    new_chunk->is_free = 1;
    new_chunk->next = allocated_chunk->next;
    new_chunk->prev = allocated_chunk;

    if (allocated_chunk->next){
        allocated_chunk->next->prev = new_chunk;
    }

    allocated_chunk->next = new_chunk;
    allocated_chunk->size = size;
}

void ch_coalesce_chunk(Chunk *chunk){

    // forward merge
    if (chunk->next && chunk->next->is_free){
        chunk->size += CHUNK_HEADER_SIZE + chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next){
            chunk->next->prev = chunk;
        }

    }

    // backward merge
    if (chunk->prev && chunk->prev->is_free){
        chunk->prev->size += CHUNK_HEADER_SIZE + chunk->size;
        chunk->prev->next = chunk->next;        
        if (chunk->next){
            chunk->next->prev = chunk->prev;
        }

    }
}

void ch_free(void *ptr){
    if (!ptr) return;

    Chunk *chunk = (Chunk *)ptr - 1;
    chunk->is_free = 1;
    ch_coalesce_chunk(chunk);
}

void* ch_alloc(size_t size){
    if (size == 0) return NULL;
    if (!heap_start) heap_init();

    size = ALIGN_UP(size);
    // first i need to check if there is a free chunk
    // if there is one, i simply return a pointer to it
    Chunk *free_chunk = ch_find_free_chunk(size);
    if (!free_chunk){
        // lets just not allocate a new chunk (for now)
        // in the future, we can grow the heap
        return NULL;
    }

    ch_split_chunk(free_chunk, size);
    free_chunk->is_free = 0;

    return (void *)(free_chunk + 1);
}

void* ch_realloc(void *ptr, size_t size){
    if (!ptr) return NULL;
    Chunk *chunk = (Chunk *)ptr - 1;

    // this chunk already has enough chunks
    // so no need to reallocate
    if (chunk->size >= size) return ptr;
    size = ALIGN_UP(size);

    void *new_ptr = ch_alloc(size);
    if (!new_ptr) return NULL;


    // this is essentially copying all the contents
    // (including the header of the older chunk into the new one)
    memcpy(new_ptr, ptr, chunk->size);
    ch_free(ptr);
    return new_ptr;
}

