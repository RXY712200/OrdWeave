#define WIN32_LEAN_AND_MEAN
#include "checkpoint_internal.h"
#include "ordweave.h"
#include "wal_internal.h"
#include <stdio.h>
#include <stdarg.h>
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
static int checked_format(char *out,size_t size,const char *format,...) {
 va_list args;va_start(args,format);int n=vsnprintf(out,size,format,args);va_end(args);T(n>=0 && (size_t)n<size);return n;
}
static char base[MAX_PATH];
static unsigned serial;
static void fresh(char path[MAX_PATH]) {
  checked_format(path, MAX_PATH, "%s\\case-%u", base, serial++);
}
static uint64_t revision(OwDatabase *d) {
  uint64_t r = 0;
  OK(ow_revision(d, &r));
  return r;
}
static void cleanup(const char *root) {
  char pattern[MAX_PATH], path[MAX_PATH];
  WIN32_FIND_DATAA data;
  HANDLE h;
  checked_format(pattern, sizeof(pattern), "%s\\*", root);
  h = FindFirstFileA(pattern, &data);
  T(h != INVALID_HANDLE_VALUE);
  do {
    if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, ".."))
      continue;
    checked_format(path, sizeof(path), "%s\\%s", root, data.cFileName);
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
  checked_format(pattern, sizeof(pattern), "%s\\checkpoints\\*.cp", path);
  h = FindFirstFileA(pattern, &f);
  if (h == INVALID_HANDLE_VALUE)
    return 0;
  do {
    ++n;
  } while (FindNextFileA(h, &f));
  FindClose(h);
  return n;
}
static unsigned segments(const char *root) {
  char pattern[MAX_PATH];
  WIN32_FIND_DATAA data;
  HANDLE h;
  unsigned n = 0;
  checked_format(pattern, sizeof(pattern), "%s\\wal\\*.wal", root);
  h = FindFirstFileA(pattern, &data);
  if (h == INVALID_HANDLE_VALUE)
    return 0;
  do {
    ++n;
  } while (FindNextFileA(h, &data));
  FindClose(h);
  return n;
}
static void populate(OwDatabase *db, OwRecordId *record,
                     OwCollectionId *collection) {
  OwRecordId deleted;
  OwOccurrenceId occurrence;
  OwValue v = {0};
  OK(ow_record_create(db, record));
  OK(ow_record_create(db, &deleted));
  OK(ow_record_delete(db, deleted));
  OK(ow_collection_create(db, collection));
  for (unsigned i = 0; i < 30; i++) {
    v.type = OW_INT;
    v.as.integer = i;
    OK(ow_field_set(db, *record, "counter", &v));
    if (i % 5 == 0)
      OK(ow_occurrence_insert(db, *collection, *record, OW_BACK,
                              (OwOccurrenceId){0}, &occurrence));
  }
}
static void crash_root(const char *path) {
  char exe[MAX_PATH], command[3 * MAX_PATH];
  STARTUPINFOA si = {0};
  PROCESS_INFORMATION pi;
  DWORD result;
  T(GetModuleFileNameA(NULL, exe, MAX_PATH));
  checked_format(command, sizeof(command), "\"%s\" crash-root \"%s\"", exe, path);
  si.cb = sizeof(si);
  T(CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
                   NULL, &si, &pi));
  T(WaitForSingleObject(pi.hProcess, 30000) == WAIT_OBJECT_0);
  T(GetExitCodeProcess(pi.hProcess, &result));
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  T(result == 0);
}
int main(int argc, char **argv) {
  OwDatabase *db = NULL, *backup = NULL;
  OwRecordId record, next;
  OwCollectionId collection;
  OwHistory *view = NULL;
  OwHistoryRange range;
  OwValue value = {0}, out;
  uint64_t latest, rev;
  char path[MAX_PATH], dest[MAX_PATH];
  unsigned before, after;
  if (argc == 3) {
    OwStatus status = ow_database_open(argv[2], &db);
    if (!strcmp(argv[1], "probe")) {
      if (status == OW_OK) {
        status = ow_verify_database(db, &range);
        if (status == OW_OK)
          printf("%llu %llu\n", (unsigned long long)range.earliest_revision,
                 (unsigned long long)range.current_revision);
        ow_database_close(db);
      }
      return status;
    }
    if (!strcmp(argv[1], "crash-root")) {
      if (status != OW_OK)
        return status;
      ow_test_fault = 44;
      T(ow_history_cleanup(db, &range) == OW_RECOVERY_REQUIRED);
      TerminateProcess(GetCurrentProcess(), 0);
    }
    if (status != OW_OK && strcmp(argv[1], "fixture"))
      return status;
    if (!strcmp(argv[1], "fixture")) {
      ow_test_segment_target = 1024;
      OK(ow_database_create_persistent(argv[2], &db));
      populate(db, &record, &collection);
      OK(ow_history_policy(db, OW_RETAIN_LAST_N, 5));
      OK(ow_history_cleanup(db, &range));
      printf("%llu %llu\n", (unsigned long long)range.earliest_revision,
             (unsigned long long)range.current_revision);
      return ow_database_close(db);
    }
  }
  {
    char cwd[MAX_PATH];
    T(GetCurrentDirectoryA(MAX_PATH, cwd));
    checked_format(base, sizeof(base), "%s\\history-%lu", cwd, GetCurrentProcessId());
    T(CreateDirectoryA(base, NULL));
  }
  ow_test_segment_target = 1024;
  fresh(path);
  OK(ow_database_create_persistent(path, &db));
  OK(ow_history_open(db, 0, &view));
  OK(ow_history_revision(view, &rev));
  T(rev == 0);
  ow_history_close(view);
  populate(db, &record, &collection);
  OK(ow_field_index_enable(db,"counter",4,8));
  latest = revision(db);
  /* RC_FACADE_COVERAGE_BEGIN: explicit historical wrapper assertions. */
  {
    OwHistory *current=NULL;size_t n=0;OwCollectionId c={0};OwRecordId r={0},*ids=NULL;
    int order=0;OwValue last={.type=OW_INT,.as.integer=29};
    OK(ow_history_open(db,latest,&current));
    OK(ow_history_collection_count(current,collection,&n));T(n==6);
    OK(ow_history_occurrence_get(current,(OwOccurrenceId){1},&c,&r));
    T(c.value==collection.value&&r.value==record.value);
    OK(ow_history_occurrence_compare(current,(OwOccurrenceId){1},(OwOccurrenceId){6},&order));T(order<0);
    OK(ow_history_query_equal(current,"counter",&last,&ids,&n));
    T(n==1&&ids[0].value==record.value);ow_history_close(current);
    T(ids[0].value==record.value);ow_array_release(ids);
  }
  /* RC_FACADE_COVERAGE_END */
  before = segments(path);
  T(before > 3);
  OK(ow_history_open(db, 5, &view));
  OK(ow_history_field_get(view, record, "counter", &out));
  T(out.as.integer == 0);
  ow_value_release(&out);
  {
    OwValue zero={.type=OW_INT};OwRecordId *ids=NULL,*live=NULL;size_t n=0,ln=0;
    OK(ow_history_query_range(view,"counter",OW_INT,&zero,1,&zero,1,&ids,&n));
    OK(ow_query_range(db,"counter",OW_INT,&zero,1,&zero,1,&live,&ln));
    T(n==1&&ids[0].value==record.value&&!live&&!ln);
    ow_array_release(ids);
  }
  value.type = OW_INT;
  value.as.integer = 100;
  OK(ow_field_set(db, record, "counter", &value));
  latest++;
  OK(ow_history_field_get(view, record, "counter", &out));
  T(out.as.integer == 0);
  ow_value_release(&out);
  {
    OwRecordId *ids=NULL;size_t n=0;OwValue zero={.type=OW_INT};
    OK(ow_history_query_range(view,"counter",OW_INT,&zero,1,&zero,1,&ids,&n));
    ow_history_close(view);view=NULL;T(n==1&&ids[0].value==record.value);ow_array_release(ids);
  }
  T(ow_history_open(db, latest + 1, &view) == OW_HISTORY_UNAVAILABLE && !view);
  T(revision(db) == latest);
  before = segments(path);
  OK(ow_history_range(db, &range));
  T(range.earliest_revision == 0 && range.policy == OW_RETAIN_MANUAL);
  OK(ow_history_cleanup(db, &range));
  T(segments(path) == before);
  OK(ow_history_policy(db, OW_RETAIN_FOREVER, 0));
  OK(ow_history_cleanup(db, &range));
  T(segments(path) == before);
  fresh(dest);
  OK(ow_backup_create(db, dest));
  OK(ow_database_open(dest, &backup));
  T(revision(backup) == latest);
  OK(ow_record_create(backup, &next));
  T(next.value == 3);
  OK(ow_database_close(backup));
  T(ow_backup_create(db, dest) == OW_CONFLICT);
  {
    char nested[MAX_PATH];
    checked_format(nested, sizeof(nested), "%s\\nested", path);
    T(ow_backup_create(db, nested) == OW_INVALID);
  }
  OK(ow_history_policy(db, OW_RETAIN_LAST_N, 5));
  OK(ow_history_range(db, &range));
  T(range.reclaimable && range.earliest_revision == 0);
  OK(ow_history_cleanup(db, &range));
  after = segments(path);
  T(after < before);
  T(range.earliest_revision > 0 && range.earliest_revision <= latest - 4);
  printf("physical WAL deletion: %u -> %u; retained %llu..%llu\n", before,
         after, (unsigned long long)range.earliest_revision,
         (unsigned long long)range.current_revision);
  uint64_t oldest = range.earliest_revision;
  OK(ow_field_index_enable(db, "counter", 4, 8));
  /* Rejected sessions must not repair roots, even when a replica is missing. */
  {
    char replica[MAX_PATH];
    checked_format(replica, sizeof(replica), "%s\\recovery.current.bak", path);
    T(DeleteFileA(replica));
    ow_store_enter(ow_core_store(db));
    {
      OwRecordId *ids=(void*)1;size_t count=99;
      T(ow_query_range(db,"counter",OW_INT,NULL,1,NULL,1,&ids,&count)==OW_BUSY&&!ids&&!count);
      T(ow_field_index_enable(db,"counter",4,8)==OW_BUSY);
      T(ow_field_index_disable(db)==OW_BUSY);
    }
    T(ow_history_cleanup(db, &range) == OW_BUSY);
    T(GetFileAttributesA(replica) == INVALID_FILE_ATTRIBUTES);
    T(GetLastError() == ERROR_FILE_NOT_FOUND);
    T(range.earliest_revision == 0 && range.current_revision == 0);
    ow_store_leave(ow_core_store(db));
    OK(ow_history_cleanup(db, &range));
    T(GetFileAttributesA(replica) != INVALID_FILE_ATTRIBUTES);
    /* A flushed but unacknowledged mutation quarantines the same session. */
    value.type = OW_INT;
    value.as.integer = 101;
    ow_test_fault = 4;
    T(ow_field_set(db, record, "counter", &value) == OW_COMMIT_UNKNOWN);
    ow_test_fault = 0;
    T(DeleteFileA(replica));
    T(ow_history_cleanup(db, &range) == OW_RECOVERY_REQUIRED);
    {
      OwRecordId *ids=(void*)1;size_t count=99;
      T(ow_query_range(db,"counter",OW_INT,NULL,1,NULL,1,&ids,&count)==OW_RECOVERY_REQUIRED&&!ids&&!count);
      T(ow_field_index_enable(db,"counter",4,8)==OW_RECOVERY_REQUIRED);
      T(ow_field_index_disable(db)==OW_RECOVERY_REQUIRED);
    }
    T(GetFileAttributesA(replica) == INVALID_FILE_ATTRIBUTES);
    T(GetLastError() == ERROR_FILE_NOT_FOUND);
    T(range.earliest_revision == 0 && range.current_revision == 0);
    T(ow_database_close(db) == OW_RECOVERY_REQUIRED);
    OK(ow_database_open(path, &db));
    OK(ow_history_cleanup(db, &range));
    T(GetFileAttributesA(replica) != INVALID_FILE_ATTRIBUTES);
    latest = revision(db);
  }
  T(ow_history_open(db, oldest - 1, &view) == OW_HISTORY_UNAVAILABLE);
  for (uint64_t i = oldest; i <= latest; i++) {
    OK(ow_history_open(db, i, &view));
    OK(ow_history_revision(view, &rev));
    T(rev == i);
    ow_history_close(view);
  }
  OK(ow_database_close(db));
  OK(ow_database_open(path, &db));
  T(revision(db) == latest);
  OK(ow_record_create(db, &next));
  T(next.value == 3);
  latest++;
  OK(ow_checkpoint_create(db));
  T(generations(path) <= 3);
  OK(ow_verify_database(db, &range));
  fresh(dest);
  OK(ow_backup_create(db, dest));
  OK(ow_database_close(db));
  {
    char offline[MAX_PATH];
    checked_format(offline, sizeof(offline), "%s.offline", path);
    T(MoveFileExA(path, offline, MOVEFILE_WRITE_THROUGH));
  }
  OK(ow_database_open(dest, &backup));
  T(revision(backup) == latest);
  OK(ow_history_open(backup, oldest, &view));
  ow_history_close(view);
  OK(ow_record_create(backup, &next));
  T(next.value == 4);
  OK(ow_database_close(backup));
  fresh(path);
  OK(ow_database_create_persistent(path, &db));
  populate(db, &record, &collection);
  latest = revision(db);
  OK(ow_history_policy(db, OW_RETAIN_LAST_N, 5));
  OK(ow_database_close(db));
  crash_root(path);
  OK(ow_database_open(path, &db));
  T(revision(db) == latest);
  OK(ow_history_cleanup(db, &range));
  OK(ow_database_close(db));
  /* Every failure leaves the source committed state recoverable. */
  for (unsigned fault = 40; fault <= 48; fault++) {
    fresh(path);
    OK(ow_database_create_persistent(path, &db));
    populate(db, &record, &collection);
    latest = revision(db);
    OK(ow_history_policy(db, OW_RETAIN_LAST_N, 5));
    ow_test_fault = fault;
    OwStatus status = ow_history_cleanup(db, &range);
    ow_test_fault = 0;
    T(status == OW_OK || status == OW_IO || status == OW_RECOVERY_REQUIRED);
    ow_database_close(db);
    OK(ow_database_open(path, &db));
    T(revision(db) == latest);
    OK(ow_history_cleanup(db, &range));
    OK(ow_database_close(db));
  }
  fresh(path);
  OK(ow_database_create_persistent(path, &db));
  populate(db, &record, &collection);
  latest = revision(db);
  for (unsigned fault = 50; fault <= 52; fault++) {
    fresh(dest);
    ow_test_fault = fault;
    T(ow_backup_create(db, dest) == OW_IO);
    ow_test_fault = 0;
    T(revision(db) == latest);
    T(ow_database_open(dest, &backup) == OW_RECOVERY_REQUIRED && !backup);
  }
  for (unsigned i = 0; i < 1000; i++) {
    ow_test_checkpoint_alloc = i;
    OwStatus status = ow_history_open(db, latest, &view);
    ow_test_checkpoint_alloc = -1;
    if (status == OW_OK) {
      ow_history_close(view);
      break;
    }
    T(status == OW_NOMEM && !view);
    T(i < 999);
  }

  OK(ow_history_policy(db, OW_RETAIN_LAST_N, 5));
  for (unsigned fail = 0; fail < 8; fail++) {
    ow_test_checkpoint_alloc = fail;
    OwStatus status = ow_history_cleanup(db, &range);
    ow_test_checkpoint_alloc = -1;
    T(status == OW_NOMEM || status == OW_RECOVERY_REQUIRED);
    if (status == OW_RECOVERY_REQUIRED) {
      ow_database_close(db);
      OK(ow_database_open(path, &db));
    }
    T(revision(db) == latest);
  }
  OK(ow_database_close(db));
  fresh(path);
  OK(ow_database_create_persistent(path, &db));
  OK(ow_record_create(db, &record));
  OK(ow_collection_create(db, &collection));
  {
    OwOccurrenceId a, b;
    OK(ow_occurrence_insert(db, collection, record, OW_BACK,
                            (OwOccurrenceId){0}, &a));
    OK(ow_occurrence_insert(db, collection, record, OW_BACK,
                            (OwOccurrenceId){0}, &b));
    struct {
      int value, front, has;
    } model[300] = {{0}};
    uint32_t rng = 17;
    unsigned current = 4;
    int front = 1, val = 0, has = 0;
    model[4].front = 1;
    OK(ow_history_policy(db, OW_RETAIN_LAST_N, 10));
    for (unsigned step = 0; step < 120; step++) {
      rng = rng * 1664525u + 1013904223u;
      unsigned kind = rng % 3;
      uint64_t old = revision(db);
      if (kind == 0) {
        val = (int)(rng % 100000);
        value.type = OW_INT;
        value.as.integer = val;
        OK(ow_field_set(db, record, "random", &value));
        has = 1;
      } else {
        int target = kind == 1;
        OK(ow_occurrence_move(db, a, target ? OW_FRONT : OW_BACK,
                              (OwOccurrenceId){0}));
        front = target;
      }
      current = (unsigned)revision(db);
      T(current < 300);
      if (current != old) {
        model[current].value = val;
        model[current].front = front;
        model[current].has = has;
      }
      if (step % 20 == 19) {
        OK(ow_history_cleanup(db, &range));
      }
      OK(ow_history_range(db, &range));
      unsigned pick = (unsigned)range.earliest_revision +
                      (rng % (current - (unsigned)range.earliest_revision + 1));
      if (pick < 4)
        pick = 4;
      OK(ow_history_open(db, pick, &view));
      OwStatus status = ow_history_field_get(view, record, "random", &out);
      if (model[pick].has) {
        OK(status);
        T(out.as.integer == model[pick].value);
        ow_value_release(&out);
      } else
        T(status == OW_NOT_FOUND);
      OwOccurrenceId *members = NULL;
      size_t count;
      OK(ow_history_collection_members(view, collection, &members, &count));
      T(count == 2 &&
        members[0].value == (model[pick].front ? a.value : b.value));
      ow_array_release(members);
      ow_history_close(view);
    }
    fresh(dest);
    OK(ow_backup_create(db, dest));
    OK(ow_database_open(dest, &backup));
    OK(ow_field_get(backup, record, "random", &out));
    T(out.as.integer == val);
    ow_value_release(&out);
    OK(ow_database_close(backup));
  }
  OK(ow_database_close(db));
  cleanup(base);
  ow_test_segment_target = OW_SEGMENT_TARGET;
  puts("history, root cleanup, independent backups, verification and fault "
       "paths verified");
  return 0;
}
