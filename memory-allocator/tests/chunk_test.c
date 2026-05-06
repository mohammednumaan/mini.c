#include "./unity/unity.h"
#include "../include/chunk.h"
#include <string.h>

static Chunk *payload_to_chunk(void *ptr) {
    return (Chunk *)((char *)ptr - CHUNK_HEADER_SIZE);
}

static void *chunk_to_payload(Chunk *chunk) {
    return (void *)((char *)chunk + CHUNK_HEADER_SIZE);
}

static Chunk *assert_alloc_and_get_chunk(size_t size) {
    void *mem = ch_alloc(size);
    TEST_ASSERT_NOT_NULL(mem);
    Chunk *chunk = payload_to_chunk(mem);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(size), chunk->size);
    return chunk;
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
    TEST_ASSERT_NULL(chunk->phys_next);
    TEST_ASSERT_NULL(chunk->phys_prev);
}

void test_should_allocate_correctly() {
    Chunk *chunk = assert_alloc_and_get_chunk(16);
    TEST_ASSERT_NOT_NULL(chunk->phys_next);
    TEST_ASSERT_NULL(chunk->phys_prev);
}

void test_should_grow_heap_when_no_space_in_current_arena() {
    Chunk *first_arena = assert_alloc_and_get_chunk(HEAP_SIZE - CHUNK_HEADER_SIZE);
    void *mem = ch_alloc(1);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *grown_chunk = payload_to_chunk(mem);
    TEST_ASSERT_NOT_EQUAL(first_arena, grown_chunk);
    TEST_ASSERT_NULL(grown_chunk->phys_prev);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(1), grown_chunk->size);
}

void test_should_allocate_chunk_larger_than_default_heap_size() {
    size_t requested = HEAP_SIZE + 128;
    void *mem = ch_alloc(requested);
    TEST_ASSERT_NOT_NULL(mem);

    Chunk *chunk = payload_to_chunk(mem);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(requested), chunk->size);
    TEST_ASSERT_NULL(chunk->phys_prev);
    TEST_ASSERT_NOT_NULL(chunk->phys_next);
    TEST_ASSERT_EQUAL_INT(1, chunk->phys_next->is_free);
}

void test_should_split_chunk_correctly() {
    Chunk *chunk = assert_alloc_and_get_chunk(16);
    Chunk *next_chunk = chunk->phys_next;
    TEST_ASSERT_NOT_NULL(next_chunk);

    TEST_ASSERT_EQUAL_INT(1, next_chunk->is_free);
    TEST_ASSERT_EQUAL_PTR(chunk, next_chunk->phys_prev);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE - ALIGN_UP(16) - CHUNK_HEADER_SIZE, next_chunk->size);
}

void test_should_not_split_exact_fit_chunk() {
    Chunk *chunk = assert_alloc_and_get_chunk(HEAP_SIZE - CHUNK_HEADER_SIZE);
    TEST_ASSERT_NULL(chunk->phys_next);
    TEST_ASSERT_NULL(chunk->phys_prev);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE, chunk->size);
}

void test_should_coalesce_forward_correctly() {
    Chunk *chunk1 = assert_alloc_and_get_chunk(16);
    Chunk *chunk2 = assert_alloc_and_get_chunk(16);

    ch_free(chunk_to_payload(chunk2));
    Chunk *merged = chunk1->phys_next;
    TEST_ASSERT_NOT_NULL(chunk1);
    TEST_ASSERT_NOT_NULL(merged);

    TEST_ASSERT_EQUAL_INT(0, chunk1->is_free);
    TEST_ASSERT_EQUAL_INT(1, merged->is_free);

    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(16), chunk1->size);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE - ALIGN_UP(16) - CHUNK_HEADER_SIZE, merged->size);
}

void test_should_coalesce_backward_correctly() {
    Chunk *chunk1 = assert_alloc_and_get_chunk(16);
    Chunk *chunk2 = assert_alloc_and_get_chunk(16);
    Chunk *chunk3 = assert_alloc_and_get_chunk(16);

    ch_free(chunk_to_payload(chunk1));
    ch_free(chunk_to_payload(chunk2));

    Chunk *merged = chunk3->phys_prev;

    TEST_ASSERT_NOT_NULL(merged);
    TEST_ASSERT_EQUAL_INT(1, merged->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(16) + CHUNK_HEADER_SIZE + ALIGN_UP(16), merged->size);
    TEST_ASSERT_EQUAL_PTR(chunk3, merged->phys_next);
    TEST_ASSERT_NULL(merged->phys_prev);
}

void test_should_coalesce_both_sides_correctly() {
    Chunk *chunk1 = assert_alloc_and_get_chunk(16);
    Chunk *chunk2 = assert_alloc_and_get_chunk(16);
    Chunk *chunk3 = assert_alloc_and_get_chunk(16);

    ch_free(chunk_to_payload(chunk1));
    ch_free(chunk_to_payload(chunk3));
    ch_free(chunk_to_payload(chunk2));

    Chunk *merged = ch_find_free_chunk(HEAP_SIZE - CHUNK_HEADER_SIZE);
    TEST_ASSERT_NOT_NULL(merged);
    TEST_ASSERT_EQUAL_PTR(chunk1, merged);
    TEST_ASSERT_EQUAL_size_t(HEAP_SIZE - CHUNK_HEADER_SIZE, merged->size);
    TEST_ASSERT_NULL(merged->phys_next);
    TEST_ASSERT_NULL(merged->phys_prev);
}

void test_should_reallocate_and_preserve_data() {
    Chunk *chunk = assert_alloc_and_get_chunk(16);
    memset(chunk_to_payload(chunk), 0xAB, 16);

    void *new_ptr = ch_realloc(chunk_to_payload(chunk), 64);
    TEST_ASSERT_NOT_NULL(new_ptr);
    TEST_ASSERT_EQUAL_INT(0, memcmp(chunk_to_payload(chunk), new_ptr, 16));

    Chunk *new_chunk = payload_to_chunk(new_ptr);
    TEST_ASSERT_EQUAL_INT(0, new_chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(64), new_chunk->size);
}

void test_should_reallocate_in_place_when_chunk_is_big_enough() {
    Chunk *chunk = assert_alloc_and_get_chunk(17);
    void *same_ptr = ch_realloc(chunk_to_payload(chunk), 22);
    TEST_ASSERT_NOT_NULL(same_ptr);
    TEST_ASSERT_EQUAL_PTR(chunk_to_payload(chunk), same_ptr);
}

void test_should_align_size_correctly() {
    assert_alloc_and_get_chunk(17);
}

void test_should_reallocate_null_pointer_like_alloc() {
    void *ptr = ch_realloc(NULL, 32);
    TEST_ASSERT_NOT_NULL(ptr);

    Chunk *chunk = payload_to_chunk(ptr);
    TEST_ASSERT_EQUAL_INT(0, chunk->is_free);
    TEST_ASSERT_EQUAL_size_t(ALIGN_UP(32), chunk->size);
}

void test_should_return_null_when_allocating_zero_bytes() {
    TEST_ASSERT_NULL(ch_alloc(0));
}

void test_should_handle_free_null_pointer() {
    ch_free(NULL);
    void *mem = ch_alloc(16);
    TEST_ASSERT_NOT_NULL(mem);
}

void test_should_reuse_freed_chunk_in_grown_arena() {
    assert_alloc_and_get_chunk(HEAP_SIZE - CHUNK_HEADER_SIZE);

    void *mem = ch_alloc(32);
    TEST_ASSERT_NOT_NULL(mem);
    ch_free(mem);

    void *reused = ch_alloc(32);
    TEST_ASSERT_EQUAL_PTR(mem, reused);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_should_init_heap_correctly);
    RUN_TEST(test_should_allocate_correctly);
    RUN_TEST(test_should_grow_heap_when_no_space_in_current_arena);
    RUN_TEST(test_should_allocate_chunk_larger_than_default_heap_size);
    RUN_TEST(test_should_split_chunk_correctly);
    RUN_TEST(test_should_not_split_exact_fit_chunk);
    RUN_TEST(test_should_coalesce_forward_correctly);
    RUN_TEST(test_should_coalesce_backward_correctly);
    RUN_TEST(test_should_coalesce_both_sides_correctly);
    RUN_TEST(test_should_reallocate_and_preserve_data);
    RUN_TEST(test_should_reallocate_in_place_when_chunk_is_big_enough);
    RUN_TEST(test_should_align_size_correctly);
    RUN_TEST(test_should_reallocate_null_pointer_like_alloc);
    RUN_TEST(test_should_return_null_when_allocating_zero_bytes);
    RUN_TEST(test_should_handle_free_null_pointer);
    RUN_TEST(test_should_reuse_freed_chunk_in_grown_arena);
    return UNITY_END();
}
