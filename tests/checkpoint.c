#define WIN32_LEAN_AND_MEAN
#include "checkpoint_internal.h"
#include "ordweave.h"
#include "wal_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define T(x)                                                                   \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "%d: %s (win=%lu)\n", __LINE__, #x, GetLastError());     \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
#define OK(x) T((x) == OW_OK)
static char base[MAX_PATH];
static unsigned serial;
static void fresh(char path[MAX_PATH]) {
  snprintf(path, MAX_PATH, "%s\\case-%u", base, serial++);
}
static uint64_t revision(OwDatabase *d) {
  uint64_t r = 0;
  OK(ow_revision(d, &r));
  return r;
}
static void check_members(OwDatabase *d, OwCollectionId c, OwOccurrenceId *a,
                          size_t n) {
  OwOccurrenceId *out = NULL;
  size_t count, i;
  OK(ow_collection_members(d, c, &out, &count));
  T(count == n);
  for (i = 0; i < n; i++)
    T(out[i].value == a[i].value);
  ow_array_release(out);
}
static void cleanup(const char *root) {
  char pattern[MAX_PATH], path[MAX_PATH];
  WIN32_FIND_DATAA data;
  HANDLE h;
  snprintf(pattern, sizeof(pattern), "%s\\*", root);
  h = FindFirstFileA(pattern, &data);
  T(h != INVALID_HANDLE_VALUE);
  do {
    if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, ".."))
      continue;
    snprintf(path, sizeof(path), "%s\\%s", root, data.cFileName);
    T(!(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT));
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      cleanup(path);
    else
      T(DeleteFileA(path));
  } while (FindNextFileA(h, &data));
  FindClose(h);
  {
    unsigned retry;
    for (retry = 0; retry < 100; retry++) {
      if (RemoveDirectoryA(root))
        break;
      Sleep(2);
    }
    T(retry < 100);
  }
}
static unsigned generations(const char *path) {
  char pattern[MAX_PATH];
  WIN32_FIND_DATAA f;
  HANDLE h;
  unsigned n = 0;
  snprintf(pattern, sizeof(pattern), "%s\\checkpoints\\*.cp", path);
  h = FindFirstFileA(pattern, &f);
  if (h == INVALID_HANDLE_VALUE)
    return 0;
  do {
    ++n;
  } while (FindNextFileA(h, &f));
  FindClose(h);
  return n;
}
int main(int argc, char **argv) {
  OwDatabase *d = NULL;
  OwRecordId r, deleted, next;
  OwCollectionId c, empty, other;
  OwOccurrenceId a, b, x;
  OwValue v = {0}, out;
  char path[MAX_PATH];
  uint64_t gen, rev, before;
  unsigned i;
  if (argc == 3) {
    if (!strcmp(argv[1], "probe")) {
      ow_test_checkpoint_restores = ow_test_checkpoint_tail = 0;
      OwStatus status = ow_database_open(argv[2], &d);
      if (status == OW_OK) {
        printf("%llu %llu %llu\n", (unsigned long long)revision(d),
               (unsigned long long)ow_test_checkpoint_restores,
               (unsigned long long)ow_test_checkpoint_tail);
        ow_database_close(d);
      }
      return status;
    }
    if (!strcmp(argv[1], "fixture")) {
      OK(ow_database_create_persistent(argv[2], &d));
      OK(ow_record_create(d, &r));
      OK(ow_collection_create(d, &c));
      OK(ow_occurrence_insert(d, c, r, OW_BACK, (OwOccurrenceId){0}, &a));
      OK(ow_occurrence_insert(d, c, r, OW_BACK, (OwOccurrenceId){0}, &b));
      OK(ow_checkpoint_create(d));
      OK(ow_record_create(d, &next));
      return ow_database_close(d);
    }
  }
  {
    char cwd[MAX_PATH];
    T(GetCurrentDirectoryA(MAX_PATH, cwd));
    snprintf(base, sizeof(base), "%s\\checkpoint-%lu", cwd,
             GetCurrentProcessId());
    T(CreateDirectoryA(base, NULL));
  }
  OK(ow_database_create(&d));
  T(ow_checkpoint_create(d) == OW_INVALID);
  T(ow_checkpoint_retention(d, 2) == OW_INVALID);
  ow_database_destroy(d);
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  OK(ow_database_close(d));
  OK(ow_database_open(path, &d));
  OK(ow_record_create(d, &r));
  OK(ow_record_create(d, &deleted));
  OK(ow_record_delete(d, deleted));
  OK(ow_collection_create(d, &c));
  OK(ow_collection_create(d, &empty));
  OK(ow_collection_create(d, &other));
  OK(ow_occurrence_insert(d, c, r, OW_BACK, (OwOccurrenceId){0}, &a));
  OK(ow_occurrence_insert(d, c, r, OW_FRONT, (OwOccurrenceId){0}, &b));
  OK(ow_occurrence_insert(d, other, r, OW_BACK, (OwOccurrenceId){0}, &x));
  for (i = OW_NULL; i <= OW_BINARY; i++) {
    char name[12];
    snprintf(name, sizeof(name), "field%u", i);
    memset(&v, 0, sizeof(v));
    v.type = (OwType)i;
    if (i == OW_BOOL)
      v.as.boolean = 1;
    if (i == OW_INT)
      v.as.integer = INT64_MIN;
    if (i == OW_FLOAT)
      v.as.real = -0.5;
    if (i == OW_STRING) {
      v.as.bytes.data = "a\0\xe4\xb8\xad";
      v.as.bytes.size = 5;
    }
    if (i == OW_BINARY) {
      v.as.bytes.data = "\xff\0";
      v.as.bytes.size = 2;
    }
    OK(ow_field_set(d, r, name, &v));
  }
  before = revision(d);
  OK(ow_checkpoint_create(d));
  T(revision(d) == before);
  OK(ow_checkpoint_info(d, &gen, &rev));
  T(gen == 1 && rev == before);
  OK(ow_record_create(d, &next));
  T(next.value == 3);
  ow_test_checkpoint_restores = ow_test_checkpoint_tail = 0;
  OK(ow_database_close(d));
  OK(ow_database_open(path, &d));
  T(ow_test_checkpoint_restores == 1 && ow_test_checkpoint_tail == 1);
  T(revision(d) == before + 1);
  {
    OwOccurrenceId order[] = {b, a};
    check_members(d, c, order, 2);
    check_members(d, empty, NULL, 0);
    check_members(d, other, &x, 1);
  }
  for (i = OW_NULL; i <= OW_BINARY; i++) {
    char name[12];
    snprintf(name, sizeof(name), "field%u", i);
    OK(ow_field_get(d, r, name, &out));
    T(out.type == (OwType)i);
    if (i == OW_BOOL)
      T(out.as.boolean == 1);
    if (i == OW_INT)
      T(out.as.integer == INT64_MIN);
    if (i == OW_FLOAT)
      T(out.as.real == -0.5);
    if (i == OW_STRING)
      T(out.as.bytes.size == 5 &&
        !memcmp(out.as.bytes.data, "a\0\xe4\xb8\xad", 5));
    if (i == OW_BINARY)
      T(out.as.bytes.size == 2 && !memcmp(out.as.bytes.data, "\xff\0", 2));
    ow_value_release(&out);
  }
  for (i = 0; i < 5; i++) {
    v.type=OW_INT;v.as.integer=i;OK(ow_field_set(d,r,"cycle",&v));
    OK(ow_checkpoint_create(d));
    T(generations(path) == 2);
    OK(ow_database_close(d));
    OK(ow_database_open(path, &d));
  }
  T(ow_checkpoint_retention(d, 0) == OW_INVALID);
  T(ow_checkpoint_retention(d, 4) == OW_INVALID);
  OK(ow_checkpoint_retention(d, 3));
  OK(ow_checkpoint_create(d));
  T(generations(path) == 3);
  OK(ow_checkpoint_retention(d, 1));
  OK(ow_checkpoint_create(d));
  T(generations(path) == 1);
  before=revision(d)-1;
  /* Exhaust every explicit export/import allocation boundary until success. */
  for (i = 0; i < 1000; i++) {
    ow_test_checkpoint_alloc = i;
    OwStatus status = ow_checkpoint_create(d);
    ow_test_checkpoint_alloc = -1;
    if (status == OW_OK)
      break;
    if (status == OW_RECOVERY_REQUIRED) {
      ow_database_close(d);
      OK(ow_database_open(path, &d));
    } else {
      if (status != OW_NOMEM)
        fprintf(stderr, "oom boundary %u status %d\n", i, status);
      T(status == OW_NOMEM);
    }
    T(revision(d) == before + 1);
  }
  T(i < 1000);
  OK(ow_database_close(d));
  for (i = 0; i < 1000; i++) {
    ow_test_checkpoint_alloc = i;
    OwStatus status = ow_database_open(path, &d);
    ow_test_checkpoint_alloc = -1;
    if (status == OW_OK) {
      OK(ow_database_close(d));
      break;
    }
    T(status == OW_NOMEM && !d);
  }
  T(i < 1000);
  for (i = 20; i <= 25; i++) {
    fresh(path);
    OK(ow_database_create_persistent(path, &d));
    OK(ow_record_create(d, &r));
    OK(ow_checkpoint_create(d));
    OK(ow_record_create(d, &next));
    ow_test_fault = i;
    OwStatus status = ow_checkpoint_create(d);
    ow_test_fault = 0;
    T(status == OW_IO || status == OW_RECOVERY_REQUIRED);
    ow_database_close(d);
    OK(ow_database_open(path, &d));
    T(revision(d) == 2);
    OK(ow_record_create(d, &r));
    OK(ow_checkpoint_create(d));
    OK(ow_database_close(d));
  }
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  OK(ow_record_create(d, &r));
  OK(ow_checkpoint_create(d));
  OK(ow_checkpoint_create(d));
  ow_test_fault = 27;
  T(ow_checkpoint_create(d) == OW_IO);
  ow_test_fault = 0;
  OK(ow_database_close(d));
  OK(ow_database_open(path, &d));
  T(revision(d) == 1);
  OK(ow_checkpoint_create(d));
  T(generations(path) == 2);
  OK(ow_database_close(d));
  fresh(path);OK(ow_database_create_persistent(path,&d));OK(ow_record_create(d,&r));
  OK(ow_collection_create(d,&c));OK(ow_collection_create(d,&other));OK(ow_collection_delete(d,other));
  OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&a));
  OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&b));OK(ow_occurrence_remove(d,b));
  OK(ow_checkpoint_create(d));OK(ow_database_close(d));OK(ow_database_open(path,&d));
  OK(ow_collection_create(d,&other));T(other.value==3);
  OK(ow_occurrence_insert(d,c,r,OW_BACK,(OwOccurrenceId){0},&b));T(b.value==3);OK(ow_database_close(d));
  cleanup(base);
  puts("checkpoint native restore, fields, counters, retention, OOM and "
       "publication faults verified");
  return 0;
}
