#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "../include/chunk.h"

static Chunk *heap_start = NULL;
static Chunk *bins[TOTAL_BINS];

void heap_init(void){
    void *memory = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(memory != MAP_FAILED);

    heap_start = (Chunk *)memory;
    heap_start->size = HEAP_SIZE - CHUNK_HEADER_SIZE;
    heap_start->is_free = 1;
    heap_start->phys_next = NULL;
    heap_start->phys_prev = NULL;
    heap_start->free_next = NULL;
    heap_start->free_prev = NULL;

    for (int i = 0; i < TOTAL_BINS; i++){
        bins[i] = NULL;
    }

    add_to_bin(heap_start);
}

static int get_bin_index(size_t size){
    if (size == 0) return 0;
    if (size <= 1023){
        int idx = (int)(size / 128);
        if (idx >= NUM_SMALL_BINS){
            idx = NUM_SMALL_BINS - 1;
        }
        return idx;
    }
    int log2 = 0;
    size_t temp_size = size;
    while (temp_size >>= 1){
        log2++;
    }

    int idx = NUM_SMALL_BINS + log2 - 10;
    if (idx >= TOTAL_BINS){
        idx = TOTAL_BINS - 1;
    }
    return idx;
}

static void add_to_bin(Chunk *chunk){
    int idx = get_bin_index(chunk->size);
    chunk->free_next = bins[idx];

    if (bins[idx]){
        bins[idx]->free_prev = chunk;
    }

    chunk->free_prev = NULL;
    bins[idx] = chunk;
}

static void remove_from_bin(Chunk *chunk){
    int idx = get_bin_index(chunk->size);

    if (chunk->free_prev){
        chunk->free_prev->free_next = chunk->free_next;
    } else {
        bins[idx] = chunk->free_next;
    }

    if (chunk->free_next){
        chunk->free_next->free_prev = chunk->free_prev;
    }

    chunk->free_next = NULL;
    chunk->free_prev = NULL;
}

static size_t round_up_to_page(size_t size){
    return ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
}

static int grow_heap(size_t size){
    size_t needed = ALIGN_UP(size) + CHUNK_HEADER_SIZE;
    size_t new_heap_size = HEAP_SIZE;

    if (needed > new_heap_size){
        new_heap_size = round_up_to_page(needed);
    }
    void *memory = mmap(NULL, new_heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (memory == MAP_FAILED){
        return 0;
    }

    Chunk *new_chunk = (Chunk *)memory;
    new_chunk->size = new_heap_size - CHUNK_HEADER_SIZE;
    new_chunk->is_free = 1;
    new_chunk->phys_next = NULL;
    new_chunk->phys_prev = NULL;
    new_chunk->free_next = NULL;
    new_chunk->free_prev = NULL;

    add_to_bin(new_chunk);
    return 1;
}
Chunk* ch_find_free_chunk(size_t size){
    int idx = get_bin_index(size);
    Chunk *best_fit = NULL;

    for (int i = idx; i < TOTAL_BINS; i++){
        Chunk *current = bins[i];
        while (current){
            if (current->is_free && current->size >= size){
                if (!best_fit || current->size < best_fit->size){
                    best_fit = current;
                }
            }
            current = current->free_next;
        }
    }
    return best_fit;
}



void ch_split_chunk(Chunk *allocated_chunk, size_t size){
    if (allocated_chunk->size < size + CHUNK_HEADER_SIZE + ALIGN) return;

    size_t remaining = allocated_chunk->size - size - CHUNK_HEADER_SIZE;

    // this is the memory available after "size" bytes
    // [HEADER][....size....][-----free-space-----]
    Chunk *new_chunk = (Chunk *)((char *)(allocated_chunk + 1) + size);
    new_chunk->size = remaining;
    new_chunk->is_free = 1;
    new_chunk->phys_next = allocated_chunk->phys_next;
    new_chunk->phys_prev = allocated_chunk;
    new_chunk->free_next = NULL;
    new_chunk->free_prev = NULL;

    if (allocated_chunk->phys_next){
        allocated_chunk->phys_next->phys_prev = new_chunk;
    }

    allocated_chunk->phys_next = new_chunk;
    allocated_chunk->size = size;
    add_to_bin(new_chunk);
}

void ch_coalesce_chunk(Chunk *chunk){

    // forward merge
    if (chunk->phys_next && chunk->phys_next->is_free){
        Chunk *next = chunk->phys_next;
        remove_from_bin(next);
        chunk->size += CHUNK_HEADER_SIZE + next->size;
        chunk->phys_next = next->phys_next;
        if (chunk->phys_next){
            chunk->phys_next->phys_prev = chunk;
        }
    }

    // backward merge
    if (chunk->phys_prev && chunk->phys_prev->is_free){
        Chunk *prev = chunk->phys_prev;
        remove_from_bin(prev);
        prev->size += CHUNK_HEADER_SIZE + chunk->size;
        prev->phys_next = chunk->phys_next;
        if (chunk->phys_next){
            chunk->phys_next->phys_prev = prev;
        }
        chunk = prev;
    }

    add_to_bin(chunk);
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
        int success = grow_heap(size);
        if (!success) return NULL;
        free_chunk = ch_find_free_chunk(size);
        if (!free_chunk) return NULL;
    }

    remove_from_bin(free_chunk);
    ch_split_chunk(free_chunk, size);
    free_chunk->is_free = 0;

    return (void *)(free_chunk + 1);
}

void* ch_realloc(void *ptr, size_t size){
    if (!ptr) return ch_alloc(size);
    if (size == 0){
        ch_free(ptr);
        return NULL;
    }
    Chunk *chunk = (Chunk *)ptr - 1;

    size = ALIGN_UP(size);

    // this chunk already has enough chunks
    // so no need to reallocate
    if (chunk->size >= size) return ptr;

    void *new_ptr = ch_alloc(size);
    if (!new_ptr) return NULL;


    // this is essentially copying all the contents
    // (including the header of the older chunk into the new one)
    memcpy(new_ptr, ptr, chunk->size);
    ch_free(ptr);
    return new_ptr;
}
