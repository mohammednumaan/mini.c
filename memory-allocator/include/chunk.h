#include <stdio.h>

#define PAGES 256
#define CHUNK_HEADER_SIZE sizeof(Chunk)
#define PAGE_SIZE 4096
#define HEAP_SIZE (PAGE_SIZE * PAGES)

typedef struct Chunk {
    size_t size;
    int is_free;
    struct Chunk *next;
    struct Chunk *prev;
} Chunk;

void heap_init();
void ch_split_chunk(Chunk *chunk, size_t size);
void ch_coalesce_chunk(Chunk *chunk);
void ch_free(void *ptr);

void* ch_alloc(size_t size);
Chunk* ch_find_free_chunk(size_t size);
