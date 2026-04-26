#include "./unity/unity.h"
#include "../include/chunk.h"
#include <string.h>

static Chunk *payload_to_chunk(void *ptr) {
    return (Chunk *)((char *)ptr - CHUNK_HEADER_SIZE);
}

void setUp() {
    heap_init();
}

void tearDown() {
}

void test_should_init_heap_correctly() {
    Chunk *chunk = ch_find_free_chunk(1);
    TEST_ASSERT_NOT_NULL(chunk);
    TEST_ASSERT_EQUAL_INT(1, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE, chunk->size);
    TEST_ASSERT_NULL(chunk->next);
    TEST_ASSERT_NULL(chunk->prev);
}

void test_should_allocate_correctly() {
    void *mem = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = payload_to_chunk(mem);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(16, chunk->size);

    TEST_ASSERT_NOT_NULL(chunk->next);
    TEST_ASSERT_NULL(chunk->prev);
}

void test_should_return_null_if_no_space() {
    for (int i = 0; i < PAGES; i++) {
        void *mem = ch_alloc(PAGE_SIZE - CHUNK_HEADER_SIZE);
        TEST_ASSERT_NOT_NULL(mem);
    }

    void *mem = ch_alloc(1);
    TEST_ASSERT_NULL(mem);
}

void test_should_split_chunk_correctly() {
    void *mem = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = payload_to_chunk(mem);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(16, chunk->size);

    Chunk *next_chunk = chunk->next;
    TEST_ASSERT_NOT_NULL(next_chunk);

    TEST_ASSERT_EQUAL_INT(1, next_chunk->is_free);
    TEST_ASSERT_EQUAL_PTR(chunk, next_chunk->prev);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE - 16 - CHUNK_HEADER_SIZE, next_chunk->size);
}

void test_should_not_split_exact_fit_chunk() {
    void *mem = ch_alloc(HEAP_SIZE - CHUNK_HEADER_SIZE);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = payload_to_chunk(mem);
    TEST_ASSERT_NULL(chunk->next);
    TEST_ASSERT_NULL(chunk->prev);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE, chunk->size);
}

void test_should_coalesce_forward_correctly() {
    void *mem1 = ch_alloc(16);
    void *mem2 = ch_alloc(16);

    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);

    ch_free(mem2);
    Chunk *chunk1 = payload_to_chunk(mem1);
    Chunk *chunk2 = chunk1->next;
    TEST_ASSERT_NOT_NULL(chunk1);
    TEST_ASSERT_NOT_NULL(chunk2);

    TEST_ASSERT_EQUAL_INT(0, chunk1->is_free);
    TEST_ASSERT_EQUAL_INT(1, chunk2->is_free);

    TEST_ASSERT_EQUAL_size_t(16, chunk1->size);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE - 16 - CHUNK_HEADER_SIZE, chunk2->size);
}

void test_should_coalesce_backward_correctly() {
    void *mem1 = ch_alloc(16);
    void *mem2 = ch_alloc(16);
    void *mem3 = ch_alloc(16);

    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    TEST_ASSERT_NOT_NULL(mem3);

    ch_free(mem1);
    ch_free(mem2);

    Chunk *chunk3 = payload_to_chunk(mem3);
    Chunk *merged = chunk3->prev;

    TEST_ASSERT_NOT_NULL(merged);
    TEST_ASSERT_EQUAL_INT(1, merged->is_free);
    TEST_ASSERT_EQUAL_size_t(16 + CHUNK_HEADER_SIZE + 16, merged->size);
    TEST_ASSERT_EQUAL_PTR(chunk3, merged->next);
    TEST_ASSERT_NULL(merged->prev);
}

void test_should_coalesce_both_sides_correctly() {
    void *mem1 = ch_alloc(16);
    void *mem2 = ch_alloc(16);
    void *mem3 = ch_alloc(16);

    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    TEST_ASSERT_NOT_NULL(mem3);

    ch_free(mem1);
    ch_free(mem3);
    ch_free(mem2);

    Chunk *chunk1 = ch_find_free_chunk(HEAP_SIZE - CHUNK_HEADER_SIZE);
    TEST_ASSERT_NOT_NULL(chunk1);
    TEST_ASSERT_EQUAL_PTR(payload_to_chunk(mem1), chunk1);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE, chunk1->size);
    TEST_ASSERT_NULL(chunk1->next);
    TEST_ASSERT_NULL(chunk1->prev);
}

void test_should_reallocate_and_preserve_data() {
    void *mem1 = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem1);
    memset(mem1, 0xAB, 16);

    void *new_ptr = ch_realloc(mem1, 64);
    TEST_ASSERT_NOT_NULL(new_ptr);
    TEST_ASSERT_EQUAL_INT(0, memcmp(mem1, new_ptr, 16));

    Chunk *new_chunk = payload_to_chunk(new_ptr);
    TEST_ASSERT_EQUAL_INT(0, new_chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(64), new_chunk->size);
}

void test_should_reallocate_in_place_when_chunk_is_big_enough() {
    void *mem = ch_alloc(17);
    TEST_ASSERT_NOT_NULL(mem);

    void *same_ptr = ch_realloc(mem, 22);
    TEST_ASSERT_NOT_NULL(same_ptr);
    TEST_ASSERT_EQUAL_PTR(mem, same_ptr);
}

void test_should_align_size_correctly() {
    void *mem = ch_alloc(17);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = payload_to_chunk(mem);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(17), chunk->size);
}

void test_should_return_null_when_allocating_zero_bytes() {
    TEST_ASSERT_NULL(ch_alloc(0));
}

void test_should_handle_free_null_pointer() {
    ch_free(NULL);

    void *mem = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_should_init_heap_correctly);
    RUN_TEST(test_should_allocate_correctly);
    RUN_TEST(test_should_return_null_if_no_space);
    RUN_TEST(test_should_split_chunk_correctly);
    RUN_TEST(test_should_not_split_exact_fit_chunk);
    RUN_TEST(test_should_coalesce_forward_correctly);
    RUN_TEST(test_should_coalesce_backward_correctly);
    RUN_TEST(test_should_coalesce_both_sides_correctly);
    RUN_TEST(test_should_reallocate_and_preserve_data);
    RUN_TEST(test_should_reallocate_in_place_when_chunk_is_big_enough);
    RUN_TEST(test_should_align_size_correctly);
    RUN_TEST(test_should_return_null_when_allocating_zero_bytes);
    RUN_TEST(test_should_handle_free_null_pointer);
    return UNITY_END();
}
