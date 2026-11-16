#define WIN32_LEAN_AND_MEAN
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
static void file_path(char *out, const char *root, const char *suffix) {
  checked_format(out, MAX_PATH, "%s\\%s", root, suffix);
}
static unsigned char *load(const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  long n;
  unsigned char *p;
  T(f);
  T(!fseek(f, 0, SEEK_END));
  n = ftell(f);
  T(n >= 0);
  rewind(f);
  p = malloc((size_t)n + 1);
  T(p);
  T(fread(p, 1, (size_t)n, f) == (size_t)n);
  fclose(f);
  *size = (size_t)n;
  return p;
}
static void save(const char *path, const unsigned char *p, size_t n) {
  FILE *f = fopen(path, "wb");
  T(f);
  T(fwrite(p, 1, n, f) == n);
  T(!fclose(f));
}
static void child(const char *mode, const char *path, DWORD expected) {
  char exe[MAX_PATH], command[3 * MAX_PATH];
  STARTUPINFOA si = {0};
  PROCESS_INFORMATION pi;
  DWORD code;
  T(GetModuleFileNameA(NULL, exe, MAX_PATH));
  checked_format(command, sizeof(command), "\"%s\" %s \"%s\"", exe, mode, path);
  si.cb = sizeof(si);
  T(CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL,
                   NULL, &si, &pi));
  T(WaitForSingleObject(pi.hProcess, 30000) == WAIT_OBJECT_0);
  T(GetExitCodeProcess(pi.hProcess, &code));
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  T(code == expected);
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
  T(RemoveDirectoryA(root));
}
int main(int argc, char **argv) {
  char path[MAX_PATH], file[MAX_PATH];
  OwDatabase *d = NULL, *other = NULL;
  OwRecordId r, s;
  OwCollectionId c, c2;
  OwOccurrenceId a, b, e, f, zero = {0};
  OwValue v = {0}, out;
  uint64_t rev;
  unsigned char id[16], again[16];
  size_t i;
  if (argc == 3) {
    OwStatus status;
    if (!strcmp(argv[1], "fixture")) {
      OK(ow_database_create_persistent(argv[2], &d));
      OK(ow_record_create(d, &r));
      return ow_database_close(d);
    }
    status = ow_database_open(argv[2], &d);
    if (!strcmp(argv[1], "busy"))
      return status == OW_BUSY ? 0 : 1;
    if (!strcmp(argv[1], "probe")) {
      if (status == OW_OK) {
        printf("%llu\n", (unsigned long long)revision(d));
        ow_database_close(d);
      }
      return (int)status;
    }
    if (status != OW_OK)
      return 2;
    if (!strcmp(argv[1], "crash")) {
      OK(ow_record_create(d, &r));
      TerminateProcess(GetCurrentProcess(), 0);
    }
    if (!strcmp(argv[1], "partial")) {
      ow_test_fault = 2;
      T(ow_record_create(d, &r) == OW_COMMIT_UNKNOWN);
      TerminateProcess(GetCurrentProcess(), 0);
    }
    if (!strcmp(argv[1], "flushed")) {
      ow_test_fault = 4;
      T(ow_record_create(d, &r) == OW_COMMIT_UNKNOWN);
      TerminateProcess(GetCurrentProcess(), 0);
    }
    return 3;
  }
  {
    char cwd[MAX_PATH];
    T(GetCurrentDirectoryA(MAX_PATH, cwd));
    checked_format(base, MAX_PATH, "%s\\persistent-%lu", cwd, GetCurrentProcessId());
    T(CreateDirectoryA(base, NULL));
  }
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  T(revision(d) == 0);
  OK(ow_database_identity(d, id));
  T(ow_database_create_persistent(path, &other) == OW_CONFLICT && !other);
  child("busy", path, 0);
  OK(ow_database_close(d));
  OK(ow_database_open(path, &d));
  OK(ow_database_identity(d, again));
  T(!memcmp(id, again, 16));
  T(revision(d) == 0);
  OK(ow_record_create(d, &r));
  OK(ow_record_create(d, &s));
  OK(ow_collection_create(d, &c));
  OK(ow_collection_create(d, &c2));
  for (i = OW_NULL; i <= OW_BINARY; i++) {
    char name[20];
    checked_format(name, sizeof(name), "type%zu", i);
    memset(&v, 0, sizeof(v));
    v.type = (OwType)i;
    if (i == OW_BOOL)
      v.as.boolean = 1;
    if (i == OW_INT)
      v.as.integer = INT64_MIN;
    if (i == OW_FLOAT)
      v.as.real = -3.25;
    if (i == OW_STRING) {
      v.as.bytes.data = "a\0\xf0\x9f\x98\x80";
      v.as.bytes.size = 6;
    }
    if (i == OW_BINARY) {
      v.as.bytes.data = "\0\xff";
      v.as.bytes.size = 2;
    }
    OK(ow_field_set(d, r, name, &v));
  }
  v.type = OW_FLOAT;
  v.as.real = -0.0;
  OK(ow_field_set(d, r, "zero", &v));
  rev = revision(d);
  v.as.real = 0;
  OK(ow_field_set(d, r, "zero", &v));
  T(revision(d) == rev);
  OK(ow_occurrence_insert(d, c, r, OW_BACK, zero, &a));
  OK(ow_occurrence_insert(d, c, r, OW_FRONT, zero, &b));
  OK(ow_occurrence_insert(d, c, r, OW_BEFORE, a, &e));
  OK(ow_occurrence_insert(d, c, r, OW_AFTER, a, &f));
  {
    OwOccurrenceId order[] = {b, e, a, f};
    check_members(d, c, order, 4);
  }
  OK(ow_occurrence_move(d, a, OW_FRONT, zero));
  OK(ow_occurrence_move(d, a, OW_BACK, zero));
  OK(ow_occurrence_move(d, a, OW_BEFORE, e));
  OK(ow_occurrence_move(d, b, OW_AFTER, f));
  {
    OwOccurrenceId order[] = {a, e, f, b};
    check_members(d, c, order, 4);
  }
  rev = revision(d);
  OK(ow_occurrence_move(d, b, OW_BACK, zero));
  OK(ow_occurrence_move(d, a, OW_BEFORE, a));
  T(revision(d) == rev);
  {
    OwOccurrenceId x;
    OK(ow_occurrence_insert(d, c2, r, OW_BACK, zero, &x));
    T(ow_record_delete(d, r) == OW_CONFLICT);
    OK(ow_occurrence_remove(d, x));
    OK(ow_collection_delete(d, c2));
  }
  OK(ow_record_delete(d, s));
  v.type = OW_INT;
  v.as.integer = 17;
  OK(ow_field_set(d, r, "replacement", &v));
  v.as.integer = 18;
  OK(ow_field_set(d, r, "replacement", &v));
  OK(ow_field_remove(d, r, "replacement"));
  rev = revision(d);
  OK(ow_database_close(d));
  OK(ow_database_open(path, &d));
  T(revision(d) == rev);
  for (i = OW_NULL; i <= OW_BINARY; i++) {
    char name[20];
    checked_format(name, sizeof(name), "type%zu", i);
    OK(ow_field_get(d, r, name, &out));
    T(out.type == (OwType)i);
    if (i == OW_INT)
      T(out.as.integer == INT64_MIN);
    if (i == OW_FLOAT)
      T(out.as.real == -3.25);
    if (i == OW_BOOL)
      T(out.as.boolean == 1);
    if (i == OW_STRING)
      T(out.as.bytes.size == 6 &&
        !memcmp(out.as.bytes.data, "a\0\xf0\x9f\x98\x80", 6));
    if (i == OW_BINARY)
      T(out.as.bytes.size == 2 && !memcmp(out.as.bytes.data, "\0\xff", 2));
    ow_value_release(&out);
  }
  T(ow_field_get(d, r, "replacement", &out) == OW_NOT_FOUND);
  {
    OwOccurrenceId order[] = {a, e, f, b};
    check_members(d, c, order, 4);
  }
  OK(ow_record_create(d, &s));
  T(s.value == 3);
  OK(ow_collection_create(d, &c2));
  T(c2.value == 3);
  OK(ow_database_close(d));
  /* Crash and uncertainty boundaries run in separate processes. */
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  OK(ow_database_close(d));
  child("crash", path, 0);
  OK(ow_database_open(path, &d));
  T(revision(d) == 1);
  OK(ow_database_close(d));
  child("partial", path, 0);
  OK(ow_database_open(path, &d));
  T(revision(d) == 1);
  OK(ow_record_create(d, &r));
  T(r.value == 2);
  OK(ow_database_close(d));
  child("flushed", path, 0);
  OK(ow_database_open(path, &d));
  T(revision(d) == 3);
  OK(ow_database_close(d));
  for (i = 1; i <= 4; i++) {
    fresh(path);
    OK(ow_database_create_persistent(path, &d));
    OK(ow_record_create(d, &r));
    ow_test_fault = (int)i;
    T(ow_record_create(d, &s) == OW_COMMIT_UNKNOWN && !s.value);
    ow_test_fault = 0;
    T(ow_revision(d, &rev) == OW_RECOVERY_REQUIRED && rev == 0);
    T(ow_record_create(d, &s) == OW_RECOVERY_REQUIRED);
    T(ow_database_close(d) == OW_RECOVERY_REQUIRED);
    OK(ow_database_open(path, &d));
    rev = revision(d);
    T(rev == (i <= 2 ? 1 : 2));
    OK(ow_record_create(d, &s));
    T(s.value == rev + 1);
    OK(ow_database_close(d));
  }
  for (i = 5; i <= 7; i++) {
    fresh(path);
    ow_test_segment_target = 300;
    OK(ow_database_create_persistent(path, &d));
    OK(ow_record_create(d, &r));
    ow_test_fault = (int)i;
    T(ow_record_create(d, &s) == OW_COMMIT_UNKNOWN && !s.value);
    ow_test_fault = 0;
    T(ow_database_close(d) == OW_RECOVERY_REQUIRED);
    OK(ow_database_open(path, &d));
    T(revision(d) == 1);
    OK(ow_record_create(d, &s));
    T(s.value == 2);
    OK(ow_database_close(d));
  }
  ow_test_segment_target = OW_SEGMENT_TARGET;
  /* Deterministic randomized persistent sequence vs independent array model,
   * repeatedly close/open rather than trusting only the live core state. */
  fresh(path);
  ow_test_segment_target = 1024;
  OK(ow_database_create_persistent(path, &d));
  OK(ow_record_create(d, &r));
  OK(ow_collection_create(d, &c));
  {
    OwOccurrenceId model[64];
    size_t count = 0, step;
    uint32_t rng = 91;
    for (step = 0; step < 500; step++) {
      size_t pos, anchor, at;
      unsigned op, place;
      OwOccurrenceId oid, aid = {0};
      rng = rng * 1664525u + 1013904223u;
      op = rng % 3;
      rng = rng * 1664525u + 1013904223u;
      place = rng % 4;
      rng = rng * 1664525u + 1013904223u;
      pos = count ? rng % count : 0;
      rng = rng * 1664525u + 1013904223u;
      anchor = count ? rng % count : 0;
      if (!count) {
        op = 0;
        place = OW_BACK;
      }
      if (count == 64 && !op)
        op = 2;
      if (place >= 2)
        aid = model[anchor];
      if (!op) {
        OK(ow_occurrence_insert(d, c, r, (OwPlacement)place, aid, &oid));
        at = place == OW_FRONT  ? 0
             : place == OW_BACK ? count
                                : anchor + (place == OW_AFTER);
        memmove(model + at + 1, model + at, (count - at) * sizeof(*model));
        model[at] = oid;
        count++;
      } else if (op == 2) {
        OK(ow_occurrence_remove(d, model[pos]));
        memmove(model + pos, model + pos + 1,
                (count - pos - 1) * sizeof(*model));
        count--;
      } else {
        oid = model[pos];
        at = place == OW_FRONT  ? 0
             : place == OW_BACK ? count
                                : anchor + (place == OW_AFTER);
        OK(ow_occurrence_move(d, oid, (OwPlacement)place, aid));
        if (!(place >= 2 && pos == anchor) && at != pos && at != pos + 1) {
          memmove(model + pos, model + pos + 1,
                  (count - pos - 1) * sizeof(*model));
          if (at > pos)
            at--;
          memmove(model + at + 1, model + at,
                  (count - 1 - at) * sizeof(*model));
          model[at] = oid;
        }
      }
      if (step % 17 == 0) {
        rev = revision(d);
        OK(ow_database_close(d));
        OK(ow_database_open(path, &d));
        T(revision(d) == rev);
      }
      check_members(d, c, model, count);
    }
  }
  OK(ow_database_close(d));
  ow_test_segment_target = OW_SEGMENT_TARGET;
  /* Real production segment target, not only the reduced test threshold. */
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  OK(ow_record_create(d, &r));
  {
    unsigned char *large = malloc(OW_FIELD_LIMIT);
    T(large);
    memset(large, 0x5a, OW_FIELD_LIMIT);
    v.type = OW_BINARY;
    v.as.bytes.data = large;
    v.as.bytes.size = OW_FIELD_LIMIT;
    for (i = 0; i < 66; i++) {
      large[0] = (unsigned char)i;
      OK(ow_field_set(d, r, "large", &v));
    }
    free(large);
    OK(ow_database_close(d));
    file_path(file, path, "wal\\00000000000000000002.wal");
    T(GetFileAttributesA(file) != INVALID_FILE_ATTRIBUTES);
    OK(ow_database_open(path, &d));
    T(revision(d) == 67);
    OK(ow_field_get(d, r, "large", &out));
    T(out.as.bytes.size == OW_FIELD_LIMIT &&
      ((const unsigned char *)out.as.bytes.data)[0] == 65);
    ow_value_release(&out);
    OK(ow_database_close(d));
  }
  /* Corruption is never repaired as an incomplete tail. */
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  OK(ow_record_create(d, &r));
  OK(ow_record_create(d, &s));
  OK(ow_database_close(d));
  file_path(file, path, "wal\\00000000000000000001.wal");
  {
    size_t len;
    unsigned char *data = load(file, &len);
    data[128 + 96] ^= 1;
    save(file, data, len);
    T(ow_database_open(path, &d) == OW_CORRUPT && !d);
    free(data);
  }
  fresh(path);
  OK(ow_database_create_persistent(path, &d));
  OK(ow_database_close(d));
  file_path(file, path, "manifest.ow");
  {
    size_t len;
    unsigned char *data = load(file, &len);
    data[11] = 4;
    save(file, data, len);
    T(ow_database_open(path, &d) == OW_UNSUPPORTED && !d);
    free(data);
  }
  T(ow_database_create_persistent("relative", &d) == OW_INVALID && !d);
  cleanup(base);
  printf("persistence, rotation, recovery, process locking/crashes, faults and "
         "model replay verified in %s\n",
         base);
  return 0;
}
