/*
 * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/common/hash_table.h>

#include <aws/common/clock.h>
#include <aws/common/string.h>
#include <aws/testing/aws_test_harness.h>
#include <stdio.h>

static const char *TEST_STR_1 = "test 1";
static const char *TEST_STR_2 = "test 2";

static const char *TEST_VAL_STR_1 = "value 1";
static const char *TEST_VAL_STR_2 = "value 2";

#define ASSERT_HASH_TABLE_ENTRY_COUNT(map, count)                                                                      \
    ASSERT_UINT_EQUALS(count, aws_hash_table_get_entry_count(map), "Hash map should have %d entries", count)

AWS_TEST_CASE(test_hash_table_put_get, s_test_hash_table_put_get_fn)
static int s_test_hash_table_put_get_fn(struct aws_allocator *allocator, void *ctx) {

    (void)ctx;

    struct aws_hash_table hash_table;
    int err_code = aws_hash_table_init(&hash_table, allocator, 10, aws_hash_c_string, aws_c_string_eq, NULL, NULL);
    struct aws_hash_element *pElem;
    int was_created;

    ASSERT_SUCCESS(err_code, "Hash Map init should have succeeded.");
    ASSERT_HASH_TABLE_ENTRY_COUNT(&hash_table, 0);

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, &pElem, &was_created);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    ASSERT_INT_EQUALS(1, was_created, "Hash Map put should have created a new element.");
    pElem->value = (void *)TEST_VAL_STR_1;

    ASSERT_HASH_TABLE_ENTRY_COUNT(&hash_table, 1);

    /* Try passing a NULL was_created this time */
    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_2, &pElem, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    pElem->value = (void *)TEST_VAL_STR_2;

    ASSERT_HASH_TABLE_ENTRY_COUNT(&hash_table, 2);

    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_1, &pElem);
    ASSERT_SUCCESS(err_code, "Hash Map get should have succeeded.");
    ASSERT_STR_EQUALS(
        TEST_VAL_STR_1,
        (const char *)pElem->value,
        "Returned value for %s, should have been %s",
        TEST_STR_1,
        TEST_VAL_STR_1);

    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_2, &pElem);
    ASSERT_SUCCESS(err_code, "Hash Map get should have succeeded.");
    ASSERT_BIN_ARRAYS_EQUALS(
        TEST_VAL_STR_2,
        strlen(TEST_VAL_STR_2) + 1,
        (const char *)pElem->value,
        strlen(pElem->value) + 1,
        "Returned value for %s, should have been %s",
        TEST_STR_2,
        TEST_VAL_STR_2);

    ASSERT_HASH_TABLE_ENTRY_COUNT(&hash_table, 2);
    aws_hash_table_clean_up(&hash_table);
    return 0;
}

AWS_TEST_CASE(test_hash_table_string_put_get, s_test_hash_table_string_put_get_fn)
static int s_test_hash_table_string_put_get_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table hash_table;
    struct aws_hash_element *pElem;
    int was_created;

    int ret = aws_hash_table_init(
        &hash_table, allocator, 10, aws_hash_string, aws_string_eq, aws_string_destroy, aws_string_destroy);
    ASSERT_SUCCESS(ret, "Hash Map init should have succeeded.");

    /* First element of hash, both key and value are statically allocated
     * strings */
    AWS_STATIC_STRING_FROM_LITERAL(key_1, "tweedle dee");
    AWS_STATIC_STRING_FROM_LITERAL(val_1, "tweedle dum");

    /* Second element of hash, only value is dynamically allocated string */
    AWS_STATIC_STRING_FROM_LITERAL(key_2, "what's for dinner?");
    const struct aws_string *val_2 = aws_string_new_from_c_str(allocator, "deadbeef");

    /* Third element of hash, only key is dynamically allocated string */
    uint8_t bytes[] = {0x88, 0x00, 0xaa, 0x13, 0xb7, 0x93, 0x7f, 0xdd, 0xbb, 0x62};
    const struct aws_string *key_3 = aws_string_new_from_array(allocator, bytes, 10);
    AWS_STATIC_STRING_FROM_LITERAL(val_3, "hunter2");

    ret = aws_hash_table_create(&hash_table, (void *)key_1, &pElem, &was_created);
    ASSERT_SUCCESS(ret, "Hash Map put should have succeeded.");
    ASSERT_INT_EQUALS(1, was_created, "Hash Map put should have created a new element.");
    pElem->value = (void *)val_1;

    /* Try passing a NULL was_created this time */
    ret = aws_hash_table_create(&hash_table, (void *)key_2, &pElem, NULL);
    ASSERT_SUCCESS(ret, "Hash Map put should have succeeded.");
    pElem->value = (void *)val_2;

    ret = aws_hash_table_create(&hash_table, (void *)key_3, &pElem, NULL);
    ASSERT_SUCCESS(ret, "Hash Map put should have succeeded.");
    pElem->value = (void *)val_3;

    ret = aws_hash_table_find(&hash_table, (void *)key_1, &pElem);
    ASSERT_SUCCESS(ret, "Hash Map get should have succeeded.");
    ASSERT_BIN_ARRAYS_EQUALS(
        "tweedle dee",
        strlen("tweedle dee"),
        aws_string_bytes(pElem->key),
        ((struct aws_string *)pElem->key)->len,
        "Returned key for %s, should have been %s",
        "tweedle dee",
        "tweedle dee");
    ASSERT_BIN_ARRAYS_EQUALS(
        "tweedle dum",
        strlen("tweedle dum"),
        aws_string_bytes(pElem->value),
        ((struct aws_string *)pElem->value)->len,
        "Returned value for %s, should have been %s",
        "tweedle dee",
        "tweedle dum");

    ret = aws_hash_table_find(&hash_table, (void *)key_2, &pElem);
    ASSERT_SUCCESS(ret, "Hash Map get should have succeeded.");
    ASSERT_BIN_ARRAYS_EQUALS(
        "what's for dinner?",
        strlen("what's for dinner?"),
        aws_string_bytes(pElem->key),
        ((struct aws_string *)pElem->key)->len,
        "Returned key for %s, should have been %s",
        "what's for dinner?",
        "what's for dinner?");
    ASSERT_BIN_ARRAYS_EQUALS(
        "deadbeef",
        strlen("deadbeef"),
        aws_string_bytes(pElem->value),
        ((struct aws_string *)pElem->value)->len,
        "Returned value for %s, should have been %s",
        "what's for dinner?",
        "deadbeef");

    ret = aws_hash_table_find(&hash_table, (void *)key_3, &pElem);
    ASSERT_SUCCESS(ret, "Hash Map get should have succeeded.");
    ASSERT_BIN_ARRAYS_EQUALS(
        bytes,
        10,
        aws_string_bytes(pElem->key),
        ((struct aws_string *)pElem->key)->len,
        "Returned key for %02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx should have been same",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[8],
        bytes[9]);
    ASSERT_BIN_ARRAYS_EQUALS(
        "hunter2",
        strlen("hunter2"),
        aws_string_bytes(pElem->value),
        ((struct aws_string *)pElem->value)->len,
        "Returned value for binary bytes should have been %s",
        "hunter2");
    aws_hash_table_clean_up(&hash_table);

    return 0;
}

AWS_TEST_CASE(test_hash_table_string_clean_up, s_test_hash_table_string_clean_up_fn)
static int s_test_hash_table_string_clean_up_fn(struct aws_allocator *allocator, void *ctx) {

    (void)ctx;

    /* Verify that clean up happens properly when a destructor function is used only on keys or only on values. */
    struct aws_hash_table hash_table;
    struct aws_hash_element *pElem;
    int was_created;

    const struct aws_string *key_1 = aws_string_new_from_c_str(allocator, "Once upon a midnight dreary,");
    AWS_STATIC_STRING_FROM_LITERAL(val_1, "while I pondered, weak and weary,");
    const struct aws_string *key_2 = aws_string_new_from_c_str(allocator, "Over many a quaint and curious");
    AWS_STATIC_STRING_FROM_LITERAL(val_2, "volume of forgotten lore--");
    const struct aws_string *key_3 = aws_string_new_from_c_str(allocator, "While I nodded, nearly napping,");
    AWS_STATIC_STRING_FROM_LITERAL(val_3, "suddenly there came a tapping,");

    const struct aws_string *dyn_keys[] = {key_1, key_2, key_3};
    const struct aws_string *static_vals[] = {val_1, val_2, val_3};

    int ret = aws_hash_table_init(
        &hash_table,
        allocator,
        10,
        aws_hash_string,
        aws_string_eq,
        aws_string_destroy,
        NULL); /* destroy keys not values */
    ASSERT_SUCCESS(ret, "Hash Map init should have succeeded.");

    for (int idx = 0; idx < 3; ++idx) {
        ret = aws_hash_table_create(&hash_table, (void *)dyn_keys[idx], &pElem, &was_created);
        ASSERT_SUCCESS(ret, "Hash Map put should have succeeded.");
        ASSERT_INT_EQUALS(1, was_created, "Hash Map put should have created a new element.");
        pElem->value = (void *)static_vals[idx];
    }

    aws_hash_table_clean_up(&hash_table);

    AWS_STATIC_STRING_FROM_LITERAL(key_4, "As of some one gently rapping,");
    const struct aws_string *val_4 = aws_string_new_from_c_str(allocator, "rapping at my chamber door.");
    AWS_STATIC_STRING_FROM_LITERAL(key_5, "\"'Tis some visitor,\" I muttered,");
    const struct aws_string *val_5 = aws_string_new_from_c_str(allocator, "\"tapping at my chamber door--");
    AWS_STATIC_STRING_FROM_LITERAL(key_6, "Only this and nothing more.\"");
    const struct aws_string *val_6 = aws_string_new_from_c_str(allocator, "from The Raven by Edgar Allan Poe (1845)");

    const struct aws_string *static_keys[] = {key_4, key_5, key_6};
    const struct aws_string *dyn_vals[] = {val_4, val_5, val_6};

    ret = aws_hash_table_init(
        &hash_table,
        allocator,
        10,
        aws_hash_string,
        aws_string_eq,
        NULL,
        aws_string_destroy); /* destroy values not keys */
    ASSERT_SUCCESS(ret, "Hash Map init should have succeeded.");

    for (int idx = 0; idx < 3; ++idx) {
        ret = aws_hash_table_create(&hash_table, (void *)static_keys[idx], &pElem, &was_created);
        ASSERT_SUCCESS(ret, "Hash Map put should have succeeded.");
        ASSERT_INT_EQUALS(1, was_created, "Hash Map put should have created a new element.");
        pElem->value = (void *)dyn_vals[idx];
    }

    aws_hash_table_clean_up(&hash_table);

    return 0;
}

static uint64_t hash_collide(const void *a) {
    (void)a;
    return 4;
}

AWS_TEST_CASE(test_hash_table_hash_collision, s_test_hash_table_hash_collision_fn)
static int s_test_hash_table_hash_collision_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table hash_table;
    struct aws_hash_element *pElem;
    int err_code = aws_hash_table_init(&hash_table, allocator, 10, hash_collide, aws_c_string_eq, NULL, NULL);

    ASSERT_SUCCESS(err_code, "Hash Map init should have succeeded.");

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, &pElem, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    pElem->value = (void *)TEST_VAL_STR_1;

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_2, &pElem, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    pElem->value = (void *)TEST_VAL_STR_2;

    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_1, &pElem);
    ASSERT_SUCCESS(err_code, "Hash Map get should have succeeded.");
    ASSERT_STR_EQUALS(
        TEST_VAL_STR_1, pElem->value, "Returned value for %s, should have been %s", TEST_STR_1, TEST_VAL_STR_1);

    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_2, &pElem);
    ASSERT_SUCCESS(err_code, "Hash Map get should have succeeded.");
    ASSERT_STR_EQUALS(
        TEST_VAL_STR_2, pElem->value, "Returned value for %s, should have been %s", TEST_STR_2, TEST_VAL_STR_2);

    aws_hash_table_clean_up(&hash_table);
    return 0;
}

AWS_TEST_CASE(test_hash_table_hash_overwrite, s_test_hash_table_hash_overwrite_fn)
static int s_test_hash_table_hash_overwrite_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table hash_table;
    struct aws_hash_element *pElem;
    int err_code = aws_hash_table_init(&hash_table, allocator, 10, aws_hash_c_string, aws_c_string_eq, NULL, NULL);
    int was_created = 42;

    ASSERT_SUCCESS(err_code, "Hash Map init should have succeeded.");

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, &pElem, &was_created); //(void *)TEST_VAL_STR_1);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    ASSERT_INT_EQUALS(1, was_created, "Hash Map create should have created a new element.");
    pElem->value = (void *)TEST_VAL_STR_1;

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, &pElem, &was_created);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    ASSERT_INT_EQUALS(0, was_created, "Hash Map create should not have created a new element.");
    ASSERT_PTR_EQUALS(TEST_VAL_STR_1, pElem->value, "Create should have returned the old value.");
    pElem->value = (void *)TEST_VAL_STR_2;

    pElem = NULL;
    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_1, &pElem);
    ASSERT_SUCCESS(err_code, "Hash Map get should have succeeded.");
    ASSERT_PTR_EQUALS(TEST_VAL_STR_2, pElem->value, "The new value should have been preserved on get");

    aws_hash_table_clean_up(&hash_table);
    return 0;
}

static void *s_last_removed_key;
static void *s_last_removed_value;
static int s_key_removal_counter = 0;
static int s_value_removal_counter = 0;

static void s_destroy_key_fn(void *key) {
    s_last_removed_key = key;
    ++s_key_removal_counter;
}
static void s_destroy_value_fn(void *value) {
    s_last_removed_value = value;
    ++s_value_removal_counter;
}

static void s_reset_destroy_ck() {
    s_key_removal_counter = 0;
    s_value_removal_counter = 0;
    s_last_removed_key = NULL;
    s_last_removed_value = NULL;
}

AWS_TEST_CASE(test_hash_table_hash_remove, s_test_hash_table_hash_remove_fn)
static int s_test_hash_table_hash_remove_fn(struct aws_allocator *allocator, void *ctx) {

    (void)ctx;

    struct aws_hash_table hash_table;
    struct aws_hash_element *pElem, elem;
    int err_code = aws_hash_table_init(
        &hash_table, allocator, 10, aws_hash_c_string, aws_c_string_eq, s_destroy_key_fn, s_destroy_value_fn);
    int was_present = 42;

    s_reset_destroy_ck();

    ASSERT_SUCCESS(err_code, "Hash Map init should have succeeded.");

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, NULL, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_2, &pElem, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    pElem->value = (void *)TEST_VAL_STR_2;

    /* Create a second time; this should not invoke destroy */
    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_2, &pElem, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");

    ASSERT_INT_EQUALS(0, s_key_removal_counter, "No keys should be destroyed at this point");
    ASSERT_INT_EQUALS(0, s_value_removal_counter, "No values should be destroyed at this point");

    err_code = aws_hash_table_remove(&hash_table, (void *)TEST_STR_1, &elem, &was_present);
    ASSERT_SUCCESS(err_code, "Hash Map remove should have succeeded.");
    ASSERT_INT_EQUALS(0, s_key_removal_counter, "No keys should be destroyed at this point");
    ASSERT_INT_EQUALS(0, s_value_removal_counter, "No values should be destroyed at this point");
    ASSERT_INT_EQUALS(1, was_present, "Item should have been removed");

    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_1, &pElem);
    ASSERT_SUCCESS(err_code, "Find for nonexistent item should still succeed");
    ASSERT_NULL(pElem, "Expected item to be nonexistent");

    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_2, &pElem);
    ASSERT_SUCCESS(err_code, "Hash Map get should have succeeded.");

    ASSERT_PTR_EQUALS(TEST_VAL_STR_2, pElem->value, "Wrong value returned from second get");

    /* If we delete and discard the element, destroy_fn should be invoked */
    err_code = aws_hash_table_remove(&hash_table, (void *)TEST_STR_2, NULL, NULL);
    ASSERT_SUCCESS(err_code, "Remove should have succeeded.");
    ASSERT_INT_EQUALS(1, s_key_removal_counter, "One key should be destroyed at this point");
    ASSERT_INT_EQUALS(1, s_value_removal_counter, "One value should be destroyed at this point");
    ASSERT_PTR_EQUALS(s_last_removed_value, TEST_VAL_STR_2, "Wrong element destroyed");

    /* If we delete an element that's not there, we shouldn't invoke destroy_fn */
    err_code = aws_hash_table_remove(&hash_table, (void *)TEST_STR_1, NULL, &was_present);
    ASSERT_SUCCESS(err_code, "Remove still should succeed on nonexistent items");
    ASSERT_INT_EQUALS(0, was_present, "Remove should indicate item not present");
    ASSERT_INT_EQUALS(1, s_key_removal_counter, "We shouldn't delete an item if none was found");
    ASSERT_INT_EQUALS(1, s_value_removal_counter, "We shouldn't delete an item if none was found");

    aws_hash_table_clean_up(&hash_table);
    return 0;
}

AWS_TEST_CASE(test_hash_table_hash_clear_allows_cleanup, s_test_hash_table_hash_clear_allows_cleanup_fn)
static int s_test_hash_table_hash_clear_allows_cleanup_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table hash_table;
    int err_code = aws_hash_table_init(
        &hash_table, allocator, 10, aws_hash_c_string, aws_c_string_eq, s_destroy_key_fn, s_destroy_value_fn);

    ASSERT_SUCCESS(err_code, "Hash Map init should have succeeded.");

    s_reset_destroy_ck();

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, NULL, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_2, NULL, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");

    aws_hash_table_clear(&hash_table);
    ASSERT_INT_EQUALS(2, s_key_removal_counter, "Clear should destroy all keys");
    ASSERT_INT_EQUALS(2, s_value_removal_counter, "Clear should destroy all values");

    struct aws_hash_element *pElem;
    err_code = aws_hash_table_find(&hash_table, (void *)TEST_STR_1, &pElem);
    ASSERT_SUCCESS(err_code, "Find should still succeed after clear");
    ASSERT_NULL(pElem, "Element should not be found");

    s_reset_destroy_ck();

    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_1, NULL, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");
    err_code = aws_hash_table_create(&hash_table, (void *)TEST_STR_2, NULL, NULL);
    ASSERT_SUCCESS(err_code, "Hash Map put should have succeeded.");

    aws_hash_table_clean_up(&hash_table);
    ASSERT_INT_EQUALS(2, s_key_removal_counter, "Cleanup should destroy all keys");
    ASSERT_INT_EQUALS(2, s_value_removal_counter, "Cleanup should destroy all values");

    return 0;
}

AWS_TEST_CASE(test_hash_table_on_resize_returns_correct_entry, s_test_hash_table_on_resize_returns_correct_entry_fn)
static int s_test_hash_table_on_resize_returns_correct_entry_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table hash_table;
    int err_code = aws_hash_table_init(&hash_table, allocator, 10, aws_hash_ptr, aws_ptr_eq, NULL, NULL);

    ASSERT_SUCCESS(err_code, "Hash Map init should have succeeded.");

    for (int i = 0; i < 20; i++) {
        struct aws_hash_element *pElem;
        int was_created;
        err_code = aws_hash_table_create(&hash_table, (void *)(intptr_t)i, &pElem, &was_created);

        ASSERT_SUCCESS(err_code, "Create should have succeeded");
        ASSERT_INT_EQUALS(1, was_created, "Create should have created new element");
        ASSERT_PTR_EQUALS(NULL, pElem->value, "New element should have null value");
        pElem->value = &hash_table;
    }

    aws_hash_table_clean_up(&hash_table);
    return 0;
}

static int s_foreach_cb_tomask(void *context, struct aws_hash_element *p_element) {
    int *p_mask = context;
    uintptr_t index = (uintptr_t)p_element->key;

    *p_mask |= (1 << index);

    return AWS_COMMON_HASH_TABLE_ITER_CONTINUE;
}

static int s_iter_count = 0;
static int s_foreach_cb_deltarget(void *context, struct aws_hash_element *p_element) {
    void **pTarget = context;
    int rv = AWS_COMMON_HASH_TABLE_ITER_CONTINUE;

    if (p_element->key == *pTarget) {
        rv |= AWS_COMMON_HASH_TABLE_ITER_DELETE;
    }
    s_iter_count++;

    return rv;
}

static int s_foreach_cb_cutoff(void *context, struct aws_hash_element *p_element) {
    (void)p_element;

    int *p_remain = context;
    s_iter_count++;
    if (--*p_remain) {
        return AWS_COMMON_HASH_TABLE_ITER_CONTINUE;
    }
    return 0;
}
static int s_foreach_cb_cutoff_del(void *context, struct aws_hash_element *p_element) {
    int *p_remain = context;
    s_iter_count++;
    if (--*p_remain) {
        return AWS_COMMON_HASH_TABLE_ITER_CONTINUE;
    }
    *p_remain = (int)(intptr_t)p_element->key;
    return AWS_COMMON_HASH_TABLE_ITER_DELETE;
}

AWS_TEST_CASE(test_hash_table_foreach, s_test_hash_table_foreach_fn)
static int s_test_hash_table_foreach_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table hash_table;
    ASSERT_SUCCESS(
        aws_hash_table_init(&hash_table, allocator, 10, aws_hash_ptr, aws_ptr_eq, NULL, NULL), "hash table init");

    for (int i = 0; i < 8; i++) {
        struct aws_hash_element *pElem;
        ASSERT_SUCCESS(aws_hash_table_create(&hash_table, (void *)(intptr_t)i, &pElem, NULL), "insert element");
        pElem->value = NULL;
    }

    // We should find all four elements
    int mask = 0;
    ASSERT_SUCCESS(aws_hash_table_foreach(&hash_table, s_foreach_cb_tomask, &mask), "foreach invocation");
    ASSERT_INT_EQUALS(0xff, mask, "bitmask");

    void *target = (void *)(uintptr_t)3;
    s_iter_count = 0;
    ASSERT_SUCCESS(aws_hash_table_foreach(&hash_table, s_foreach_cb_deltarget, &target), "foreach invocation");
    ASSERT_INT_EQUALS(8, s_iter_count, "iteration should not stop when deleting");

    mask = 0;
    ASSERT_SUCCESS(aws_hash_table_foreach(&hash_table, s_foreach_cb_tomask, &mask), "foreach invocation");
    ASSERT_INT_EQUALS(0xf7, mask, "element 3 deleted");

    s_iter_count = 0;
    int remain = 4;
    ASSERT_SUCCESS(aws_hash_table_foreach(&hash_table, s_foreach_cb_cutoff, &remain), "foreach invocation");
    ASSERT_INT_EQUALS(0, remain, "no more remaining iterations");
    ASSERT_INT_EQUALS(4, s_iter_count, "correct iteration count");

    s_iter_count = 0;
    remain = 4;

    ASSERT_SUCCESS(aws_hash_table_foreach(&hash_table, s_foreach_cb_cutoff_del, &remain), "foreach invocation");
    ASSERT_INT_EQUALS(4, s_iter_count, "correct iteration count");
    // we use remain as a side channel to report which element we deleted
    int expected_mask = 0xf7 & ~(1 << remain);

    mask = 0;
    ASSERT_SUCCESS(aws_hash_table_foreach(&hash_table, s_foreach_cb_tomask, &mask), "foreach invocation");
    ASSERT_INT_EQUALS(expected_mask, mask, "stop element deleted");

    aws_hash_table_clean_up(&hash_table);

    return 0;
}

/*
 * Convenience functions for a hash table which uses uint64_t as keys, and whose
 * hash function is just the identity function.
 */
static uint64_t s_hash_uint64_identity(const void *a) {
    return *(uint64_t *)a;
}

static bool s_hash_uint64_eq(const void *a, const void *b) {
    uint64_t my_a = *(uint64_t *)a;
    uint64_t my_b = *(uint64_t *)b;
    return my_a == my_b;
}

AWS_TEST_CASE(test_hash_table_iter, s_test_hash_table_iter_fn)
static int s_test_hash_table_iter_fn(struct aws_allocator *allocator, void *ctx) {

    (void)ctx;

    /* Table entries are: (2^0 -> 2^10), (2^1 -> 2^11), (2^2 -> 2^12), ..., (2^9 -> 2^19).
     * We will iterate through the table and AND all the keys and all the values together
     * to ensure that we have hit every element of the table.
     */

    uint64_t powers_of_2[20];
    uint64_t x = 1;
    for (int i = 0; i < 20; ++i, x <<= 1) {
        powers_of_2[i] = x;
    }

    struct aws_hash_table map;
    ASSERT_SUCCESS(
        aws_hash_table_init(&map, allocator, 10, s_hash_uint64_identity, s_hash_uint64_eq, NULL, NULL),
        "hash table init");

    struct aws_hash_element *elem;
    for (int i = 0; i < 10; ++i) {
        int ret = aws_hash_table_create(&map, (void *)(powers_of_2 + i), &elem, NULL);
        ASSERT_SUCCESS(ret, "Hash Map put should have succeeded.");
        elem->value = (void *)(powers_of_2 + 10 + i);
    }

    uint64_t keys_bitflags = 0;
    uint64_t values_bitflags = 0;
    int num_elements = 0;
    for (struct aws_hash_iter iter = aws_hash_iter_begin(&map); !aws_hash_iter_done(&iter); aws_hash_iter_next(&iter)) {
        uint64_t key = *(const uint64_t *)iter.element.key;
        uint64_t value = *(uint64_t *)iter.element.value;
        keys_bitflags |= key;
        values_bitflags |= value;
        ++num_elements;
    }
    ASSERT_INT_EQUALS(num_elements, 10);
    ASSERT_UINT_EQUALS(keys_bitflags, 0x3ffULL);     // keys are bottom 10 bits
    ASSERT_UINT_EQUALS(values_bitflags, 0xffc00ULL); // values are next 10 bits

    aws_hash_table_clean_up(&map);
    return 0;
}

AWS_TEST_CASE(test_hash_table_empty_iter, s_test_hash_table_empty_iter_fn)
static int s_test_hash_table_empty_iter_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    struct aws_hash_table map;
    ASSERT_SUCCESS(aws_hash_table_init(&map, allocator, 10, s_hash_uint64_identity, s_hash_uint64_eq, NULL, NULL));

    struct aws_hash_iter iter = aws_hash_iter_begin(&map);
    ASSERT_TRUE(aws_hash_iter_done(&iter));
    aws_hash_iter_next(&iter);
    ASSERT_TRUE(aws_hash_iter_done(&iter));

    aws_hash_table_clean_up(&map);
    return 0;
}

struct churn_entry {
    void *key;
    int original_index;
    void *value;
    int is_removed;
};

static int s_qsort_churn_entry(const void *a, const void *b) {
    const struct churn_entry *const *p1 = a, *const *p2 = b;
    const struct churn_entry *e1 = *p1, *e2 = *p2;

    if (e1->key < e2->key) {
        return -1;
    }
    if (e1->key > e2->key) {
        return 1;
    }
    if (e1->original_index < e2->original_index) {
        return -1;
    }
    if (e1->original_index > e2->original_index) {
        return 1;
    }
    return 0;
}

static long s_timestamp() {
    uint64_t time = 0;
    aws_sys_clock_get_ticks(&time);
    return (long)(time / 1000);
}

AWS_TEST_CASE(test_hash_churn, s_test_hash_churn_fn)
static int s_test_hash_churn_fn(struct aws_allocator *allocator, void *ctx) {
    (void)ctx;

    int i = 0;
    struct aws_hash_table hash_table;
    int nentries = 2 * 512 * 1024;
    int err_code = aws_hash_table_init(&hash_table, allocator, nentries, aws_hash_ptr, aws_ptr_eq, NULL, NULL);

    if (AWS_ERROR_SUCCESS != err_code) {
        FAIL("hash table creation failed: %d", err_code);
    }

    /* Probability that we deliberately try to overwrite.
       Note that random collisions can occur, and are not explicitly avoided. */
    double pOverwrite = 0.05;
    double pDelete = 0.05;

    struct churn_entry *entries = calloc(sizeof(*entries), nentries);
    struct churn_entry **permuted = calloc(sizeof(*permuted), nentries);

    for (i = 0; i < nentries; i++) {
        struct churn_entry *e = &entries[i];
        permuted[i] = e;
        e->original_index = i;

        int mode = 0; /* 0 = new entry, 1 = overwrite, 2 = delete */

        if (i != 0) {
            double p = (double)rand();
            if (p < pOverwrite) {
                mode = 1;
            } else if (p < pOverwrite + pDelete) {
                mode = 2;
            }
        }

        e->is_removed = 0;
        if (mode == 0) {
            e->key = (void *)(uintptr_t)rand();
            e->value = (void *)(uintptr_t)rand();
        } else if (mode == 1) {
            e->key = entries[(size_t)rand() % i].key; /* not evenly distributed but close enough */
            e->value = (void *)(uintptr_t)rand();
        } else if (mode == 2) {
            e->key = entries[(size_t)rand() % i].key; /* not evenly distributed but close enough */
            e->value = 0;
            e->is_removed = 1;
        }
    }

    qsort(permuted, nentries, sizeof(*permuted), s_qsort_churn_entry);

    long start = s_timestamp();

    for (i = 0; i < nentries; i++) {
        if (!(i % 100000)) {
            printf("Put progress: %d/%d\n", i, nentries);
        }
        struct churn_entry *e = &entries[i];
        if (e->is_removed) {
            int was_present;
            err_code = aws_hash_table_remove(&hash_table, e->key, NULL, &was_present);
            ASSERT_SUCCESS(err_code, "Unexpected failure removing element");
            if (i == 0 && entries[i - 1].key == e->key && entries[i - 1].is_removed) {
                ASSERT_INT_EQUALS(0, was_present, "Expected item to be missing");
            } else {
                ASSERT_INT_EQUALS(1, was_present, "Expected item to be present");
            }
        } else {
            struct aws_hash_element *pElem;
            int was_created;
            err_code = aws_hash_table_create(&hash_table, e->key, &pElem, &was_created);
            ASSERT_SUCCESS(err_code, "Unexpected failure adding element");

            pElem->value = e->value;
        }
    }

    for (i = 0; i < nentries; i++) {
        if (!(i % 100000)) {
            printf("Check progress: %d/%d\n", i, nentries);
        }
        struct churn_entry *e = permuted[i];

        if (i < nentries - 1 && permuted[i + 1]->key == e->key) {
            // overwritten on subsequent step
            continue;
        }

        struct aws_hash_element *pElem;
        aws_hash_table_find(&hash_table, e->key, &pElem);

        if (e->is_removed) {
            ASSERT_NULL(pElem, "expected item to be deleted");
        } else {
            ASSERT_NOT_NULL(pElem, "expected item to be present");
            ASSERT_PTR_EQUALS(e->value, pElem->value, "wrong value for item");
        }
    }

    aws_hash_table_clean_up(&hash_table);

    long end = s_timestamp();

    free(entries);
    free(permuted);

    printf("elapsed=%ld us\n", end - start);
    return 0;
}
