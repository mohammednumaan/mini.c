#include "./unity/unity.h"
#include "../include/chunk.h"
#include <stdio.h>

void setUp(){};
void tearDown(){};

void test_should_init_heap_correctly() {
    heap_init();
    Chunk *chunk = ch_find_free_chunk(1);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_INT(1, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE,  chunk->size);
    TEST_ASSERT_NULL(chunk->next);
    TEST_ASSERT_NULL(chunk->prev);
}

void test_should_allocate_correctly() {
    heap_init();
    void *mem = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = (Chunk *)((char *)mem - CHUNK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(16, chunk->size);

    TEST_ASSERT_NOT_NULL(chunk->next);
    TEST_ASSERT_NULL(chunk->prev);
}

void test_should_return_null_if_no_space() {
    heap_init();
    for (int i = 0; i < PAGES; i++){
        void *mem = ch_alloc(PAGE_SIZE - CHUNK_HEADER_SIZE);
        TEST_ASSERT_NOT_NULL(mem);
    }
    void *mem = ch_alloc(1);
    TEST_ASSERT_NULL(mem);
}

void test_should_split_chunk_correctly() {
    heap_init();
    void *mem = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = (Chunk *)((char *)mem - CHUNK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(16, chunk->size);

    Chunk *next_chunk = chunk->next;
    TEST_ASSERT_NOT_NULL(next_chunk);

    TEST_ASSERT_EQUAL_INT(1, next_chunk->is_free);
    TEST_ASSERT_EQUAL_PTR(chunk, next_chunk->prev);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE - 16 - CHUNK_HEADER_SIZE, next_chunk->size);
}

void test_should_coalesce_chunks_correctly() {
    heap_init();
    void *mem1 = ch_alloc(16);
    void *mem2 = ch_alloc(16);

    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);

    // [Chunk1 (allocated)] -> [Chunk2 (allocated)] -> [Chunk3 (free)]
    // free Chunk2 and check if it coalesces with Chunk3
    ch_free(mem2);
    Chunk *chunk1 = (Chunk *)((char *)mem1 - CHUNK_HEADER_SIZE);
    Chunk *chunk2 = chunk1->next;
    TEST_ASSERT_NOT_NULL(chunk1);
    TEST_ASSERT_NOT_NULL(chunk2);

    TEST_ASSERT_EQUAL_INT(0, chunk1->is_free);
    TEST_ASSERT_EQUAL_INT(1, chunk2->is_free);

    TEST_ASSERT_EQUAL_size_t(16, chunk1->size);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE - 16 - CHUNK_HEADER_SIZE, chunk2->size);
}


void test_should_reallocate_correctly() {
    heap_init();
    void *mem1 = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem1);

    void *new_ptr = ch_realloc(mem1, 32);
    TEST_ASSERT_NOT_NULL(new_ptr);

    Chunk *new_chunk = (Chunk *)((char *)new_ptr - CHUNK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_INT(0, new_chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(32, new_chunk->size);

    Chunk *old_chunk = (Chunk *)((char *)mem1 - CHUNK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_INT(1, old_chunk->is_free);
}

void test_should_align_size_correctly() {
    heap_init();
    void *mem1 = ch_alloc(17);
    TEST_ASSERT_NOT_NULL(mem1);

    Chunk *chunk = (Chunk *)((char *)mem1 - CHUNK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(17), chunk->size);


    void *mem2 = ch_realloc(mem1, 22);
    TEST_ASSERT_NOT_NULL(mem2);
    TEST_ASSERT_EQUAL_PTR(mem1, mem2);

    void *mem3 = ch_realloc(mem2, 56);
    TEST_ASSERT_NOT_NULL(mem3);

    Chunk *chunk3 = (Chunk *)((char *)mem3 - CHUNK_HEADER_SIZE);
    TEST_ASSERT_EQUAL_INT(0, chunk3->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(56), chunk3->size);

}

int main(){
    UNITY_BEGIN();
    RUN_TEST(test_should_init_heap_correctly);
    RUN_TEST(test_should_allocate_correctly);
    RUN_TEST(test_should_return_null_if_no_space);
    RUN_TEST(test_should_split_chunk_correctly);
    RUN_TEST(test_should_coalesce_chunks_correctly);
    RUN_TEST(test_should_reallocate_correctly);
    RUN_TEST(test_should_align_size_correctly);
    return UNITY_END();
}
