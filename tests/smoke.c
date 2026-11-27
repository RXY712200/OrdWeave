#include "ordweave.h"
#include "layerkeysort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Item { const char *id; } Item;
static LksStatus associate(void *item, uint64_t ordinal, void *ctx,
                          const void **data, size_t *size)
{
    (void)ordinal; (void)ctx;
    *data = ((Item *)item)->id;
    *size = strlen(((Item *)item)->id);
    return LKS_STATUS_OK;
}
static LksStatus resolve(const void *data, size_t size, uint64_t ordinal,
                         void *ctx, void **item)
{
    Item *items = ctx;
    size_t i;
    (void)ordinal;
    *item = NULL;
    for (i = 0; i < 3; ++i) {
        if (size == strlen(items[i].id) && !memcmp(data, items[i].id, size)) {
            *item = &items[i];
            return LKS_STATUS_OK;
        }
    }
    return LKS_STATUS_NOT_FOUND;
}
/* Bounded traversal detects missing, extra, and misplaced occurrences. */
static int sequence(LksOrder *order, Item *const expected[3])
{
    const LksOrderHandle *it = lks_order_first(order);
    size_t i;
    if (lks_order_size(order) != 3) return 0;
    for (i = 0; i < 3; ++i) {
        if (!it || lks_order_item(it) != expected[i]) return 0;
        it = lks_order_next(it);
    }
    return it == NULL;
}
#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #expr); goto done; \
} } while (0)
int main(void)
{
    Item items[] = {{"item-a"}, {"item-b"}, {"item-c"}};
    Item *initial[] = {&items[0], &items[1], &items[2]};
    Item *captured[] = {&items[1], &items[2], &items[0]};
    const unsigned char ns[] = {0x4f, 0x72, 0x64, 0x57};
    LksSnapshotOptions options = {ns, sizeof(ns), associate, NULL};
    LksOrder *order = NULL, *restored = NULL;
    LksSnapshot *snapshot = NULL, *loaded = NULL;
    const LksOrderHandle *handles[3] = {NULL, NULL, NULL};
    unsigned char *bytes = NULL;
    size_t i, size = 0;
    int cmp = 0, result = EXIT_FAILURE;
    CHECK(strcmp(ordweave_version(), "1.1.0-preview.4") == 0);
    order = lks_order_create();
    CHECK(order);
    for (i = 0; i < 3; ++i)
        CHECK(lks_order_insert_back(order, &items[i], &handles[i]) == LKS_STATUS_OK);
    CHECK(sequence(order, initial));
    CHECK(lks_order_compare(order, handles[0], handles[2], &cmp) == LKS_STATUS_OK);
    CHECK(cmp < 0);
    CHECK(lks_order_move_back(order, handles[0]) == LKS_STATUS_OK);
    CHECK(sequence(order, captured));
    CHECK(lks_order_compare(order, handles[0], handles[2], &cmp) == LKS_STATUS_OK);
    CHECK(cmp > 0);
    CHECK(lks_order_snapshot_capture(order, &options, &snapshot) == LKS_STATUS_OK);
    CHECK(lks_snapshot_count(snapshot) == 3);
    size = lks_snapshot_serialized_size(snapshot);
    CHECK(size > 0);
    bytes = malloc(size);
    CHECK(bytes);
    CHECK(lks_snapshot_serialize(snapshot, bytes, size) == LKS_STATUS_OK);
    lks_order_destroy(order);
    order = NULL;
    /* Original handles have expired; only serialized application IDs are used below. */
    CHECK(lks_snapshot_deserialize(bytes, size, &loaded) == LKS_STATUS_OK);
    CHECK(lks_snapshot_count(loaded) == 3);
    CHECK(lks_snapshot_restore_order(loaded, resolve, items, &restored) == LKS_STATUS_OK);
    CHECK(sequence(restored, captured));
    puts("OrdWeave version and LKS ordering/snapshot round trip verified");
    result = EXIT_SUCCESS;
done:
    lks_order_destroy(restored);
    lks_order_destroy(order);
    lks_snapshot_destroy(loaded);
    lks_snapshot_destroy(snapshot);
    free(bytes);
    return result;
}
