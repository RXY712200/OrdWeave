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
static int audit_cases(void);
int main(int argc, char **argv) {
  OwDatabase *db = NULL;
  OwRecordId r;
  OwCollectionId c;
  OwOccurrenceId o;
  OwValue v = {0};
  OwStatus status;
  if (argc == 4 && !strcmp(argv[1], "crash")) {
    OK(ow_database_open(argv[2], &db));
    ow_test_crash = atoi(argv[3]);
    OwHistoryRange range;
    ow_history_cleanup(db, &range);
    return 99;
  }
  if (argc >= 3 && !strcmp(argv[1], "fixture")) {
    OK(ow_database_create_persistent(argv[2], &db));
    OK(ow_record_create(db, &r));
    v.type = OW_INT;
    v.as.integer = 7;
    OK(ow_field_set(db, r, "value", &v));
    OK(ow_collection_create(db, &c));
    OK(ow_occurrence_insert(db, c, r, OW_BACK, (OwOccurrenceId){0}, &o));
    OK(ow_checkpoint_create(db));
    v.as.integer = 9;
    OK(ow_field_set(db, r, "value", &v));
    return ow_database_close(db);
  }
  if (argc >= 3) {
    status = ow_database_open(argv[2], &db);
    if (status != OW_OK)
      return status;
    if (argc == 4 && !strcmp(argv[1], "backup"))
      status = ow_backup_create(db, argv[3]);
    else if (!strcmp(argv[1], "probe")) {
      OwHistoryRange range;
      status = ow_verify_database(db, &range);
    }
    ow_database_close(db);
    return status;
  }
  if (argc != 1)
    return 1;
  return audit_cases();
}
static void spawn_crash(const char *path, unsigned point) {
  char exe[MAX_PATH], command[3 * MAX_PATH];
  STARTUPINFOA si = {0};
  PROCESS_INFORMATION pi;
  DWORD code;
  T(GetModuleFileNameA(NULL, exe, MAX_PATH));
  checked_format(command, sizeof(command), "\"%s\" crash \"%s\" %u", exe, path,
           point);
  si.cb = sizeof(si);
  T(CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
                   NULL, &si, &pi));
  T(WaitForSingleObject(pi.hProcess, 30000) == WAIT_OBJECT_0);
  T(GetExitCodeProcess(pi.hProcess, &code));
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  T(code == 0);
}
static int audit_cases(void) {
  char path[MAX_PATH], dest[MAX_PATH], cwd[MAX_PATH];
  OwDatabase *db = NULL, *copy = NULL;
  OwRecordId r, removed, next;
  OwCollectionId c;
  OwOccurrenceId a, b, x;
  OwValue v = {0}, out;
  OwHistory *h = NULL;
  OwHistoryRange range;
  uint64_t rev;
  T(GetCurrentDirectoryA(MAX_PATH, cwd));
  checked_format(base, sizeof(base), "%s\\audit-%lu", cwd, GetCurrentProcessId());
  T(CreateDirectoryA(base, NULL));
  fresh(path);
  ow_test_segment_target = 700;
  OK(ow_database_create_persistent(path, &db));
  OK(ow_checkpoint_retention(db, 3));
  OK(ow_record_create(db, &r));
  OK(ow_record_create(db, &removed));
  OK(ow_record_delete(db, removed));
  OK(ow_collection_create(db, &c));
  OK(ow_occurrence_insert(db, c, r, OW_BACK, (OwOccurrenceId){0}, &a));
  OK(ow_occurrence_insert(db, c, r, OW_FRONT, (OwOccurrenceId){0}, &b));
  v.type = OW_INT;
  v.as.integer = 10;
  OK(ow_field_set(db, r, "v", &v));
  OK(ow_checkpoint_create(db));
  OK(ow_occurrence_move(db, a, OW_FRONT, (OwOccurrenceId){0}));
  v.as.integer = 20;
  OK(ow_field_set(db, r, "v", &v));
  OK(ow_checkpoint_create(db));
  OK(ow_occurrence_remove(db, b));
  OK(ow_occurrence_insert(db, c, r, OW_BACK, (OwOccurrenceId){0}, &x));
  T(x.value == 3);
  OK(ow_field_remove(db, r, "v"));
  v.as.integer = 30;
  OK(ow_field_set(db, r, "v", &v));
  OK(ow_checkpoint_create(db));
  v.as.integer = 40;
  OK(ow_field_set(db, r, "v", &v));
  T(revision(db) == 14);
  for (unsigned i = 0; i <= 14; i++) {
    int exists;
    OK(ow_history_open(db, i, &h));
    OK(ow_history_revision(h, &rev));
    T(rev == i);
    OK(ow_history_record_exists(h, removed, &exists));
    T(exists == (i == 2));
    OwStatus status = ow_history_field_get(h, r, "v", &out);
    if (i >= 7 && i != 12) {
      OK(status);
      T(out.as.integer == (i < 9 ? 10 : i < 12 ? 20 : i == 13 ? 30 : 40));
      ow_value_release(&out);
    } else
      T(status == (i == 0 ? OW_NOT_FOUND : OW_NOT_FOUND));
    if (i >= 4) {
      OwOccurrenceId *members = NULL;
      size_t count;
      OK(ow_history_collection_members(h, c, &members, &count));
      size_t expected = i < 5 ? 0 : i == 5 ? 1 : i < 10 ? 2 : i == 10 ? 1 : 2;
      T(count == expected);
      if (count)
        T(members[0].value == ((i >= 6 && i < 8) ? b.value : a.value));
      if (count == 2)
        T(members[1].value == (i >= 11 ? x.value : i < 8 ? a.value : b.value));
      ow_array_release(members);
    }
    ow_history_close(h);
    T(revision(db) == 14);
  }
  fresh(dest);
  OK(ow_backup_create(db, dest));
  OK(ow_database_open(dest, &copy));
  OK(ow_record_create(copy, &next));
  T(next.value == 3);
  OK(ow_database_close(copy));
  OK(ow_history_policy(db, OW_RETAIN_LAST_N, 4));
  OK(ow_history_cleanup(db, &range));
  T(range.earliest_revision > 0);
  T(ow_history_open(db, range.earliest_revision - 1, &h) ==
        OW_HISTORY_UNAVAILABLE &&
    !h);
  for (uint64_t i = range.earliest_revision; i <= 14; i++) {
    OK(ow_history_open(db, i, &h));
    ow_history_close(h);
  }
  OK(ow_checkpoint_create(db));
  OK(ow_checkpoint_create(db));
  T(generations(path)<=3);
  OK(ow_verify_database(db, &range));
  OK(ow_database_close(db));
  OK(ow_database_open(path, &db));
  OK(ow_record_create(db, &next));
  T(next.value == 3);
  OK(ow_database_close(db));
  for (unsigned point = 70; point <= 74; point++) {
    fresh(path);
    OK(ow_database_create_persistent(path, &db));
    OK(ow_record_create(db, &r));
    v.type = OW_INT;
    for (unsigned i = 0; i < 25; i++) {
      v.as.integer = i;
      OK(ow_field_set(db, r, "v", &v));
    }
    rev = revision(db);
    OK(ow_history_policy(db, OW_RETAIN_LAST_N, 4));
    OK(ow_database_close(db));
    spawn_crash(path, point);
    OK(ow_database_open(path, &db));
    T(revision(db) == rev);
    OK(ow_field_get(db, r, "v", &out));
    T(out.as.integer == 24);
    ow_value_release(&out);
    OK(ow_history_cleanup(db, &range));
    OK(ow_checkpoint_create(db));
    OK(ow_database_close(db));
  }
  fresh(path);
  OK(ow_database_create_persistent(path, &db));
  OK(ow_record_create(db, &r));
  fresh(dest);
  ow_test_fault = 53;
  T(ow_backup_create(db, dest) == OW_IO);
  ow_test_fault = 0;
  T(ow_database_open(dest, &copy) == OW_RECOVERY_REQUIRED && !copy);
  OK(ow_database_close(db));
  fresh(path);OK(ow_database_create_persistent(path,&db));OK(ow_record_create(db,&r));v.type=OW_STRING;v.as.bytes.data="owned";v.as.bytes.size=5;OK(ow_field_set(db,r,"copy",&v));OK(ow_history_open(db,2,&h));OK(ow_history_field_get(h,r,"copy",&out));ow_history_close(h);OK(ow_database_close(db));T(!memcmp(out.as.bytes.data,"owned",5));ow_value_release(&out);
  uint64_t invalid=99;T(ow_history_revision(NULL,&invalid)==OW_INVALID&&invalid==0);OwRecordId failed={99};T(ow_record_create(NULL,&failed)==OW_INVALID&&failed.value==0);
  {
    OwDatabase *left=NULL,*right=NULL;OwRecordId lr,rr;OwValue first={.type=OW_INT,.as.integer=1},second={.type=OW_BOOL,.as.boolean=1};
    OK(ow_database_create(&left));OK(ow_database_create(&right));OK(ow_record_create(left,&lr));OK(ow_record_create(right,&rr));
    OK(ow_field_set(left,lr,"a",&first));OK(ow_field_set(left,lr,"b",&second));OK(ow_field_set(right,rr,"b",&second));OK(ow_field_set(right,rr,"a",&first));
    ow_test_checkpoint_alloc=0;T(ow_core_same_state(left,right));ow_test_checkpoint_alloc=-1;
    first.as.integer=2;OK(ow_field_set(left,lr,"a",&first));OK(ow_field_set(right,rr,"a",&second));T(!ow_core_same_state(left,right));ow_database_destroy(left);ow_database_destroy(right);
  }
  cleanup(base);
  ow_test_segment_target = OW_SEGMENT_TARGET;
  puts("all historical checkpoint intervals and five process-kill cleanup "
       "boundaries verified");
  return 0;
}
