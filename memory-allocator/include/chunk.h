#ifndef CHUNK_H
#define CHUNK_H

#include <stdio.h>

#define ALIGN 16
#define PAGES 256

#define CHUNK_HEADER_SIZE sizeof(Chunk)
#define PAGE_SIZE 4096

#define HEAP_SIZE (PAGE_SIZE * PAGES)
#define ALIGN_UP(size) (((size) + (ALIGN - 1)) & ~(ALIGN - 1))

#define NUM_SMALL_BINS 8
#define NUM_LARGE_BINS 20 
#define TOTAL_BINS (NUM_SMALL_BINS + NUM_LARGE_BINS)

typedef struct Chunk {
    size_t size;
    int is_free;
    struct Chunk *phys_next;
    struct Chunk *phys_prev;
    struct Chunk *free_next;
    struct Chunk *free_prev;
} Chunk;

void heap_init();
void ch_split_chunk(Chunk *chunk, size_t size);
void ch_coalesce_chunk(Chunk *chunk);
void ch_free(void *ptr);

void* ch_alloc(size_t size);
void* ch_realloc(void *ptr, size_t size);
Chunk* ch_find_free_chunk(size_t size);

static int get_bin_index(size_t size);
static void add_to_bin(Chunk *chunk);
static void remove_from_bin(Chunk *chunk);
static size_t round_up_to_page(size_t size);
static int grow_heap(size_t size);


#endif
