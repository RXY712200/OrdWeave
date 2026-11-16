/* Windows primitives adapted from Obscura64
 * 3db0122560853a8f3db3ba6888067394dc1a3e62. MIT (c) 2026 RXY712200. WAL
 * append/recovery is OrdWeave-specific. */
#define WIN32_LEAN_AND_MEAN
#include "wal_internal.h"
#include "checkpoint_internal.h"
// clang-format off
// clang-format off
#include <windows.h>
#include <bcrypt.h>
// clang-format on
// clang-format on
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
struct OwStore {
  HANDLE root, directory, lock, wal;
  wchar_t *path;
  unsigned char id[16], digest[32];
  uint64_t segment, offset;
  int busy, poisoned;
  int validating;
  uint64_t scan_revision,checkpoint_revision;
  unsigned char anchor_found[32];
  unsigned char metadata[OW_CHECKPOINT_META];
  unsigned retention;
  unsigned char recovery_root[256];
  uint64_t first_segment,earliest,through;
  OwRetentionPolicy history_policy;
  uint64_t history_n;
  int readonly_scan,limit_replay,semantic_scan,allow_incomplete_tail;
  unsigned char root_anchor_found[32];
  uint64_t selected_generation,selected_revision;
};
#ifdef OW_TESTING
int ow_test_fault;
uint64_t ow_test_segment_target = OW_SEGMENT_TARGET;
#endif
static OwStatus io_status(void) {
  DWORD e = GetLastError();
  if (e == ERROR_SHARING_VIOLATION || e == ERROR_LOCK_VIOLATION)
    return OW_BUSY;
  if (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND)
    return OW_NOT_FOUND;
  return OW_IO;
}
/* Adapted dos_device_name; reject every DOS device component, ADS, relative
 * components, trailing-dot/space aliases, UNC, extended/device paths and links.
 */
static int device(const wchar_t *p, size_t n) {
  size_t k = 0;
  wchar_t a, b, c;
  while (k < n && p[k] != L'.')
    ++k;
  if (k >= 32)
    return 0;
  wchar_t stem[32];
  size_t i;
  for (i = 0; i < k; i++)
    stem[i] = towupper(p[i]);
  stem[k] = 0;
  if (!wcscmp(stem, L"CONIN$") || !wcscmp(stem, L"CONOUT$"))
    return 1;
  if (k != 3 && k != 4)
    return 0;
  a = stem[0];
  b = stem[1];
  c = stem[2];
  if (k == 3)
    return (a == L'C' && b == L'O' && c == L'N') ||
           (a == L'P' && b == L'R' && c == L'N') ||
           (a == L'A' && b == L'U' && c == L'X') ||
           (a == L'N' && b == L'U' && c == L'L');
  return ((a == L'C' && b == L'O' && c == L'M') ||
          (a == L'L' && b == L'P' && c == L'T')) &&
         ((stem[3] >= L'1' && stem[3] <= L'9') || stem[3] == 0xb9 ||
          stem[3] == 0xb2 || stem[3] == 0xb3);
}
static OwStatus safe_path(const char *utf8, wchar_t **out) {
  int n;
  wchar_t *p;
  size_t i, start;
  *out = NULL;
  if (!utf8)
    return OW_INVALID;
  n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
  if (n < 5 || n > 240)
    return OW_INVALID;
  p = malloc((size_t)n * sizeof(*p));
  if (!p)
    return OW_NOMEM;
  if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, p, n)) {
    free(p);
    return OW_INVALID;
  }
  if (!iswalpha(p[0]) || p[0] > 127 || p[1] != L':' ||
      (p[2] != L'\\' && p[2] != L'/'))
    goto invalid;
  for (i = 0; i < (size_t)n; i++)
    if (p[i] == L'/')
      p[i] = L'\\';
  start = 3;
  for (i = 3; i < (size_t)n; i++) {
    if (p[i] && (p[i] < 32 || wcschr(L"<>:\"|?*~", p[i])))
      goto invalid;
    if (!p[i] || p[i] == L'\\') {
      size_t len = i - start;
      if (!len || p[i - 1] == L' ' || p[i - 1] == L'.' ||
          device(p + start, len))
        goto invalid;
      /* Existing ancestors must be ordinary directories, no reparse aliases. */
      if (p[i]) {
        DWORD attr;
        p[i] = 0;
        attr = GetFileAttributesW(p);
        p[i] = L'\\';
        if (attr == INVALID_FILE_ATTRIBUTES ||
            !(attr & FILE_ATTRIBUTE_DIRECTORY) ||
            (attr & FILE_ATTRIBUTE_REPARSE_POINT))
          goto invalid;
      }
      start = i + 1;
    }
  }
  *out = p;
  return OW_OK;
invalid:
  free(p);
  return OW_INVALID;
}
static wchar_t *child(OwStore *s, const wchar_t *suffix) {
  size_t n = wcslen(s->path), m = wcslen(suffix);
  wchar_t *p = malloc((n + m + 1) * sizeof(*p));
  if (p) {
    memcpy(p, s->path, n * sizeof(*p));
    memcpy(p + n, suffix, (m + 1) * sizeof(*p));
  }
  return p;
}
static HANDLE open_file(OwStore *s, const wchar_t *suffix, DWORD access,
                        DWORD disposition) {
  wchar_t *p = child(s, suffix);
  HANDLE h;
  BY_HANDLE_FILE_INFORMATION info;
  if (!p) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return INVALID_HANDLE_VALUE;
  }
  h = CreateFileW(p, access, FILE_SHARE_READ | (access==GENERIC_READ?FILE_SHARE_WRITE:0), NULL, disposition,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
  free(p);
  if (h != INVALID_HANDLE_VALUE &&
      (!GetFileInformationByHandle(h, &info) ||
       (info.dwFileAttributes &
        (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) ||
       info.nNumberOfLinks != 1)) {
    CloseHandle(h);
    SetLastError(ERROR_INVALID_DATA);
    return INVALID_HANDLE_VALUE;
  }
  return h;
}
static OwStatus write_exact(HANDLE file, const unsigned char *data,
                            size_t size) {
  size_t pos = 0;
  while (pos < size) {
    DWORD request = size - pos > MAXDWORD ? MAXDWORD : (DWORD)(size - pos),
          written = 0;
    if (!WriteFile(file, data + pos, request, &written, NULL) || !written ||
        written > request)
      return OW_IO;
    pos += written;
  }
  return OW_OK;
}
static OwStatus read_exact(HANDLE file, unsigned char *data, size_t size) {
  while (size) {
    DWORD n = 0, chunk = size > MAXDWORD ? MAXDWORD : (DWORD)size;
    if (!ReadFile(file, data, chunk, &n, NULL))
      return OW_IO;
    if (!n)
      return OW_CORRUPT;
    data += n;
    size -= n;
  }
  return OW_OK;
}
static int seek(HANDLE h, uint64_t offset) {
  LARGE_INTEGER n;
  n.QuadPart = (LONGLONG)offset;
  return SetFilePointerEx(h, n, NULL, FILE_BEGIN) != 0;
}
static int file_size(HANDLE h, uint64_t *size) {
  LARGE_INTEGER n;
  if (!GetFileSizeEx(h, &n) || n.QuadPart < 0)
    return 0;
  *size = (uint64_t)n.QuadPart;
  return 1;
}
static OwStatus digest_check(const unsigned char *p, size_t n) {
  unsigned char hash[32];
  OwStatus s = ow_sha256(p, n - 32, NULL, 0, hash);
  if (s != OW_OK)
    return s;
  return memcmp(hash, p + n - 32, 32) ? OW_CORRUPT : OW_OK;
}
static void segment_name(uint64_t seq, wchar_t out[64], int pending) {
  swprintf(out, 64, L"\\wal\\%020llu.%ls", (unsigned long long)seq,
           pending ? L"pending" : L"wal");
}
static OwStatus new_segment(OwStore *s, uint64_t revision) {
  unsigned char header[OW_SEGMENT_HEADER] = {0};
  wchar_t tmp[64], name[64], *a, *b;
  HANDLE h;
  OwStatus status;
  if (s->segment == UINT64_MAX)
    return OW_CAPACITY;
  segment_name(s->segment + 1, tmp, 1);
  segment_name(s->segment + 1, name, 0);
  memcpy(header, "OWWAL003", 8);
  ow_put(header + 8, 3, 4);
  ow_put(header + 12, sizeof(header), 4);
  memcpy(header + 16, s->id, 16);
  ow_put(header + 32, s->segment + 1, 8);
  ow_put(header + 40, revision, 8);
  memcpy(header + 48, s->digest, 32);
  status = ow_sha256(header, 96, NULL, 0, header + 96);
  if (status != OW_OK)
    return status;
  h = open_file(s, tmp, GENERIC_WRITE, CREATE_NEW);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  if (OW_FAULT(5)) {
    write_exact(h, header, 17);
    CloseHandle(h);
    return OW_IO;
  }
  status = write_exact(h, header, sizeof(header));
  if (status == OW_OK && !FlushFileBuffers(h))
    status = OW_IO;
  if (!CloseHandle(h) && status == OW_OK)
    status = OW_IO;
  if (status != OW_OK)
    return status;
  if (OW_FAULT(6))
    return OW_IO;
  a = child(s, tmp);
  b = child(s, name);
  if (!a || !b) {
    free(a);
    free(b);
    return OW_NOMEM;
  }
  if (!MoveFileExW(a, b, MOVEFILE_WRITE_THROUGH)) {
    free(a);
    free(b);
    return io_status();
  }
  free(a);
  free(b);
  if (OW_FAULT(7))
    return OW_IO;
  h = open_file(s, name, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  if (s->wal != INVALID_HANDLE_VALUE)
    CloseHandle(s->wal);
  s->wal = h;
  ++s->segment;
  s->offset = sizeof(header);
  return seek(h, s->offset) ? OW_OK : OW_IO;
}
OwStatus ow_store_guard(const OwStore *s) {
  return !s            ? OW_OK
         : s->poisoned ? OW_RECOVERY_REQUIRED
         : s->busy     ? OW_BUSY
                       : OW_OK;
}
void ow_store_enter(OwStore *s) {
  if (s)
    s->busy = 1;
}
void ow_store_leave(OwStore *s) {
  if (s)
    s->busy = 0;
}
OwStatus ow_store_identity(const OwStore *s, unsigned char out[16]) {
  if (out)
    memset(out, 0, 16);
  if (!s || !out)
    return OW_INVALID;
  OwStatus status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  memcpy(out, s->id, 16);
  return OW_OK;
}
void ow_store_close(OwStore *s) {
  if (!s)
    return;
  if (s->wal != INVALID_HANDLE_VALUE)
    CloseHandle(s->wal);
  if (s->lock != INVALID_HANDLE_VALUE)
    CloseHandle(s->lock);
  if (s->directory != INVALID_HANDLE_VALUE)
    CloseHandle(s->directory);
  if (s->root != INVALID_HANDLE_VALUE)
    CloseHandle(s->root);
  free(s->path);
  free(s);
}
OwStatus ow_store_commit(OwStore *s, unsigned char *f, size_t n, OwCommand *cmd,
                         uint64_t revision) {
  OwStatus status;
  uint64_t target = OW_SEGMENT_TARGET;
#ifdef OW_TESTING
  target = ow_test_segment_target;
#endif
  memcpy(f, "OWTXN003", 8);
  ow_put(f + 8, n, 4);
  ow_put(f + 12, 3, 2);
  ow_put(f + 14, cmd->op, 2);
  ow_put(f + 16, revision, 8);
  ow_put(f + 24, n - OW_FRAME_OVERHEAD, 4);
  memcpy(f + 32, s->digest, 32);
  ow_put(f + OW_FRAME_HEADER, cmd->id, 8);
  status = ow_sha256(f, 64, NULL, 0, f + 64);
  if (status == OW_OK)
    status = ow_sha256(f, n - 40, NULL, 0, f + n - 40);
  memcpy(f + n - 8, "COMMIT03", 8);
  if (status != OW_OK)
    goto unknown;
  if (s->offset > OW_SEGMENT_HEADER &&
      (s->offset > target || n > target - s->offset)) {
    status = new_segment(s, revision);
    if (status != OW_OK)
      goto unknown;
  }
  if (OW_FAULT(1)) {
    status = OW_IO;
    goto unknown;
  }
  if (OW_FAULT(2)) {
    write_exact(s->wal, f, n / 2);
    status = OW_IO;
    goto unknown;
  }
  status = write_exact(s->wal, f, n);
  if (status != OW_OK)
    goto unknown;
  if (OW_FAULT(3)) {
    status = OW_IO;
    goto unknown;
  }
  if (!FlushFileBuffers(s->wal)) {
    status = OW_IO;
    goto unknown;
  }
  if (OW_FAULT(4)) {
    status = OW_IO;
    goto unknown;
  }
  memcpy(s->digest, f + n - 40, 32);
  s->offset += n;
  return OW_OK;
unknown:
  (void)status;
  s->poisoned = 1;
  return OW_COMMIT_UNKNOWN;
}
/* All headers and complete frames are validated before replay. Only a short
 * final header or a verified complete header with a short body is repairable.
 */
static OwStatus replay_segment(OwStore *s, OwDatabase *db, uint64_t seq,
                               int last) {
  wchar_t name[64];
  unsigned char sh[128], fh[96], *f = NULL;
  uint64_t length, offset = 128, rev;
  OwStatus status;
  HANDLE h;
  segment_name(seq, name, 0);
  h = open_file(s, name, (s->readonly_scan?GENERIC_READ:GENERIC_READ | GENERIC_WRITE), OPEN_EXISTING);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  status = OW_IO;
  if (!file_size(h, &length))
    goto done;
  if (length < 128) {
    status = OW_CORRUPT;
    goto done;
  }
  status = read_exact(h, sh, 128);
  if (status != OW_OK)
    goto done;
  if (memcmp(sh, "OWWAL003", 8)) {
    status = OW_CORRUPT;
    goto done;
  }
  if (ow_get(sh + 8, 4) != 3) {
    status = OW_UNSUPPORTED;
    goto done;
  }
  status = digest_check(sh, 128);
  if (status != OW_OK)
    goto done;
  rev=s->scan_revision;
  if (ow_get(sh + 12, 4) != 128 || memcmp(sh + 16, s->id, 16) ||
      ow_get(sh + 32, 8) != seq || rev == UINT64_MAX ||
      ow_get(sh + 40, 8) != rev + 1 || memcmp(sh + 48, s->digest, 32)) {
    status = OW_CORRUPT;
    goto done;
  }
  {
    unsigned char zero[16] = {0};
    if (memcmp(sh + 80, zero, 16)) {
      status = OW_UNSUPPORTED;
      goto done;
    }
  }
  while (offset < length) {
    size_t n;
    uint64_t before, expected;
    unsigned char hash[32];
    OwCommand cmd;
    char *field = NULL;
    OwValue value;
    if (length - offset < 96) {
      size_t short_size = (size_t)(length - offset),
             prefix = short_size < 8 ? short_size : 8;
      status = read_exact(h, fh, short_size);
      if (status != OW_OK)
        goto done;
      if (memcmp(fh, "OWTXN003", prefix)) {
        status = OW_CORRUPT;
        goto done;
      }
      goto tail;
    }
    status = read_exact(h, fh, 96);
    if (status != OW_OK)
      goto done;
    if (memcmp(fh, "OWTXN003", 8)) {
      status = OW_CORRUPT;
      goto done;
    }
    if (ow_get(fh + 12, 2) != 3) {
      status = OW_UNSUPPORTED;
      goto done;
    }
    status = digest_check(fh, 96);
    if (status != OW_OK)
      goto done;
    n = (size_t)ow_get(fh + 8, 4);
    if (n < OW_FRAME_OVERHEAD + 8 || n > OW_FRAME_LIMIT ||
        ow_get(fh + 24, 4) != n - OW_FRAME_OVERHEAD || ow_get(fh + 28, 4)) {
      status = OW_CORRUPT;
      goto done;
    }
    if (n > length - offset)
      goto tail;
    f = malloc(n);
    if (!f) {
      status = OW_NOMEM;
      goto done;
    }
    memcpy(f, fh, 96);
    status = read_exact(h, f + 96, n - 96);
    if (status != OW_OK)
      goto done;
    status = ow_sha256(f, n - 40, NULL, 0, hash);
    if (status != OW_OK)
      goto done;
    if (memcmp(hash, f + n - 40, 32) || memcmp(f + n - 8, "COMMIT03", 8) ||
        memcmp(f + 32, s->digest, 32)) {
      status = OW_CORRUPT;
      goto done;
    }
    before=s->scan_revision;
    expected = ow_get(f + 16, 8);
    if (before == UINT64_MAX || expected != before + 1) {
      status = OW_CORRUPT;
      goto done;
    }
    status = ow_decode(f + 96, n - OW_FRAME_OVERHEAD,
                       (OwOpcode)ow_get(f + 14, 2), &cmd, &field, &value);
    if (status == OW_OK && !s->validating && expected>s->checkpoint_revision) {
      if(!s->limit_replay || expected<=s->through) status = ow_apply(db, &cmd);
#ifdef OW_TESTING
      if(status==OW_OK && !s->semantic_scan && (!s->limit_replay || expected<=s->through)) ++ow_test_checkpoint_tail;
#endif
    }
    free(field);
    if (status != OW_OK) {
      if (status != OW_NOMEM && status != OW_IO)
        status = OW_CORRUPT;
      goto done;
    }
    rev=expected;
    if(!s->validating && expected>s->checkpoint_revision && (!s->limit_replay || expected<=s->through)) ow_core_revision(db,&rev);
    if (rev != expected) {
      status = OW_CORRUPT;
      goto done;
    }
    s->scan_revision=expected;
    if(expected==ow_get(s->recovery_root+152,8)) memcpy(s->root_anchor_found,hash,32);
    if(expected==s->checkpoint_revision) memcpy(s->anchor_found,hash,32);
    memcpy(s->digest, hash, 32);
    offset += n;
    free(f);
    f = NULL;
  }
  status = OW_OK;
  goto completed;
tail:
  if(s->readonly_scan && !s->allow_incomplete_tail) {status=OW_RECOVERY_REQUIRED;goto done;}
  if (!last) {
    status = OW_CORRUPT;
    goto done;
  }
  if (!s->validating && !s->readonly_scan && (!seek(h, offset) || !SetEndOfFile(h) || !FlushFileBuffers(h))) {
    status = OW_IO;
    goto done;
  }
  status = OW_OK;
completed:
  if (!last && offset == 128) {
    status = OW_CORRUPT;
    goto done;
  }
  if (last) {
    s->wal = h;
    s->segment = seq;
    s->offset = offset;
    h = INVALID_HANDLE_VALUE;
  }
done:
  free(f);
  if (h != INVALID_HANDLE_VALUE)
    CloseHandle(h);
  return status;
}
static OwStatus discover(OwStore *s, uint64_t *count) {
  wchar_t *pattern = child(s, L"\\wal\\*");
  WIN32_FIND_DATAW data;
  HANDLE find;
  uint64_t max = 0, total = 0;
  OwStatus status = OW_OK;
  if (!pattern)
    return OW_NOMEM;
  find = FindFirstFileW(pattern, &data);
  free(pattern);
  if (find == INVALID_HANDLE_VALUE)
    return io_status();
  do {
    const wchar_t *n = data.cFileName;
    size_t len = wcslen(n), i;
    uint64_t seq = 0;
    if (!wcscmp(n, L".") || !wcscmp(n, L".."))
      continue;
    if (data.dwFileAttributes &
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) {
      status = OW_CORRUPT;
      break;
    }
    if (len != 24 && len != 28) {
      status = OW_CORRUPT;
      break;
    }
    for (i = 0; i < 20; i++) {
      unsigned digit = (unsigned)(n[i] - L'0');
      if (digit > 9 || seq > (UINT64_MAX - digit) / 10)
        break;
      seq = seq * 10 + digit;
    }
    if (i != 20 || !seq ||
        (wcscmp(n + 20, L".wal") && wcscmp(n + 20, L".pending"))) {
      status = OW_CORRUPT;
      break;
    }
    if (len == 28)
      continue; /* Reconciled only after authoritative chain validates. */
    if(seq<s->first_segment) continue;
    if (seq > max) max = seq;
    ++total;
  } while (FindNextFileW(find, &data));
  if (status == OW_OK && GetLastError() != ERROR_NO_MORE_FILES)
    status = OW_IO;
  FindClose(find);
  if (status != OW_OK)
    return status;
  if (!total || max-s->first_segment+1 != total)
    return s->recovery_root[0]?OW_RECOVERY_REQUIRED:OW_CORRUPT;
  *count = max;
  return OW_OK;
}
static OwStatus clear_pending(OwStore *s, uint64_t count) {
  wchar_t name[64], *path;
  WIN32_FIND_DATAW data;
  HANDLE find;
  path = child(s, L"\\wal\\*.pending");
  if (!path)
    return OW_NOMEM;
  find = FindFirstFileW(path, &data);
  free(path);
  if (find == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND ? OW_OK : OW_IO;
  do {
    uint64_t seq = 0;
    size_t i;
    for (i = 0; i < 20; i++)
      seq = seq * 10 + (unsigned)(data.cFileName[i] - L'0');
    if (count == UINT64_MAX || seq != count + 1) {
      FindClose(find);
      return OW_CORRUPT;
    }
    segment_name(seq, name, 1);
    path = child(s, name);
    if (!path) {
      FindClose(find);
      return OW_NOMEM;
    }
    if (!DeleteFileW(path)) {
      free(path);
      FindClose(find);
      return OW_IO;
    }
    free(path);
  } while (FindNextFileW(find, &data));
  DWORD error = GetLastError();
  FindClose(find);
  return error == ERROR_NO_MORE_FILES ? OW_OK : OW_IO;
}
static OwStatus ow_store_open_internal(const char *path,int create,OwStore **out,OwDatabase *db,int allow_incomplete);
static OwStatus history_root_load(OwStore *s);
static OwStatus history_baseline(OwStore *s,OwDatabase *db);
static void history_scan_reset(OwStore *s);
#include "checkpoint_windows.inc"
#include "history_windows.inc"

static OwStatus ow_store_open_internal(const char *path, int create, OwStore **out,
                       OwDatabase *db,int allow_incomplete) {
  OwStore *s = calloc(1, sizeof(*s));
  OwDatabase *audited=NULL;
  OwStatus status;
  wchar_t *p;
  HANDLE m = INVALID_HANDLE_VALUE;
  unsigned char manifest[96] = {0};
  uint64_t size, count, i;
  OVERLAPPED range = {0};
  *out = NULL;
  if (!s)
    return OW_NOMEM;
  s->retention=2;s->first_segment=1;
  s->root = s->directory = s->lock = s->wal = INVALID_HANDLE_VALUE;
  status = safe_path(path, &s->path);
  if (status != OW_OK)
    goto done;
  if (create && !CreateDirectoryW(s->path, NULL)) {
    status = GetLastError() == ERROR_ALREADY_EXISTS ? OW_CONFLICT : io_status();
    goto done;
  }
  s->root = CreateFileW(
      s->path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      NULL);
  if (s->root == INVALID_HANDLE_VALUE) {
    status = io_status();
    goto done;
  }
  {
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(s->root, &info)) {
      status = OW_IO;
      goto done;
    }
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
      status = OW_INVALID;
      goto done;
    }
  }
  s->lock = open_file(s, L"\\writer.lock", GENERIC_READ | GENERIC_WRITE,
                      create ? CREATE_NEW : OPEN_ALWAYS);
  if (s->lock == INVALID_HANDLE_VALUE) {
    status = io_status();
    goto done;
  }
  if (!LockFileEx(s->lock, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                  0, 1, 0, &range)) {
    status = io_status();
    goto done;
  }
  p = child(s, L"\\wal");
  if (!p) {
    status = OW_NOMEM;
    goto done;
  }
  if (create && !CreateDirectoryW(p, NULL)) {
    free(p);
    status = io_status();
    goto done;
  }
  s->directory = CreateFileW(
      p, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      NULL);
  free(p);
  if (s->directory == INVALID_HANDLE_VALUE) {
    status = io_status();
    goto done;
  }
  {
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(s->directory, &info)) {
      status = OW_IO;
      goto done;
    }
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
      status = OW_CORRUPT;
      goto done;
    }
  }
  if(!create && !allow_incomplete) {wchar_t *marker=child(s,L"\\backup.incomplete");if(!marker) {status=OW_NOMEM;goto done;}DWORD a=GetFileAttributesW(marker),marker_error=GetLastError();free(marker);if(a!=INVALID_FILE_ATTRIBUTES) {status=OW_RECOVERY_REQUIRED;goto done;}if(marker_error!=ERROR_FILE_NOT_FOUND && marker_error!=ERROR_PATH_NOT_FOUND) {status=OW_IO;goto done;}}
  if (create) {
    if (!BCRYPT_SUCCESS(BCryptGenRandom(NULL, s->id, 16,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
      status = OW_IO;
      goto done;
    }
    memcpy(manifest, "OWMAN003", 8);
    ow_put(manifest + 8, 3, 4);
    ow_put(manifest + 12, 96, 4);
    memcpy(manifest + 16, s->id, 16);
    ow_put(manifest + 32, OW_SEGMENT_TARGET, 8);
    ow_put(manifest + 40, OW_FRAME_LIMIT, 8);
    status = ow_sha256(manifest, 64, NULL, 0, manifest + 64);
    if (status != OW_OK)
      goto done;
    /* Publish manifest last: an interrupted initialization is never an empty
     * DB. */
    status = new_segment(s, 1);
    if (status != OW_OK)
      goto done;
    m = open_file(s, L"\\manifest.pending", GENERIC_WRITE, CREATE_NEW);
    if (m == INVALID_HANDLE_VALUE) {
      status = io_status();
      goto done;
    }
    status = write_exact(m, manifest, 96);
    if (status == OW_OK && !FlushFileBuffers(m))
      status = OW_IO;
    CloseHandle(m);
    m = INVALID_HANDLE_VALUE;
    if (status != OW_OK)
      goto done;
    wchar_t *a = child(s, L"\\manifest.pending"),
            *b = child(s, L"\\manifest.ow");
    if (!a || !b) {
      free(a);
      free(b);
      status = OW_NOMEM;
      goto done;
    }
    if (!MoveFileExW(a, b, MOVEFILE_WRITE_THROUGH)) {
      free(a);
      free(b);
      status = io_status();
      goto done;
    }
    free(a);
    free(b);
  } else {
    m = open_file(s, L"\\manifest.ow", GENERIC_READ, OPEN_EXISTING);
    if (m == INVALID_HANDLE_VALUE) {
      status = GetLastError() == ERROR_FILE_NOT_FOUND ? OW_RECOVERY_REQUIRED
                                                      : io_status();
      goto done;
    }
    if (!file_size(m, &size)) {
      status = OW_IO;
      goto done;
    }
    if (size != 96) {
      status = OW_CORRUPT;
      goto done;
    }
    status = read_exact(m, manifest, 96);
    CloseHandle(m);
    m = INVALID_HANDLE_VALUE;
    if (status != OW_OK)
      goto done;
    if (memcmp(manifest, "OWMAN003", 8)) {
      status = OW_CORRUPT;
      goto done;
    }
    if (ow_get(manifest + 8, 4) != 3) {
      status = OW_UNSUPPORTED;
      goto done;
    }
    status = digest_check(manifest, 96);
    if (status != OW_OK)
      goto done;
    {
      unsigned char zero[16] = {0};
      if (ow_get(manifest + 12, 4) != 96 ||
          ow_get(manifest + 32, 8) != OW_SEGMENT_TARGET ||
          ow_get(manifest + 40, 8) != OW_FRAME_LIMIT ||
          memcmp(manifest + 48, zero, 16)) {
        status = OW_UNSUPPORTED;
        goto done;
      }
    }
    memcpy(s->id, manifest + 16, 16);
    status=history_root_load(s);if(status!=OW_OK) goto done;
    status = discover(s, &count);
    if (status != OW_OK)
      goto done;
    /* Pass one validates the entire chain without constructing historical state. */
    s->validating=1;history_scan_reset(s);
    for(i=s->first_segment;;i++) {status=replay_segment(s,db,i,i==count);if(status!=OW_OK) goto done;if(i==count) break;}
    status=history_semantic_state(s,s->scan_revision,&audited);if(status!=OW_OK) goto done;
    status=history_baseline(s,db);if(status!=OW_OK) goto done;
    status=checkpoint_load(s,db,count);if(status!=OW_OK) goto done;
    CloseHandle(s->wal);s->wal=INVALID_HANDLE_VALUE;
    history_scan_reset(s);s->validating=0;
    for(i=s->first_segment;;i++) {status=replay_segment(s,db,i,i==count);if(status!=OW_OK) goto done;if(i==count) break;}
    status=history_same_state(db,audited,s->id);if(status!=OW_OK) goto done;
    ow_core_database_destroy(audited);audited=NULL;
    status = clear_pending(s, count);
    if (status != OW_OK)
      goto done;
    if (!seek(s->wal, s->offset)) {
      status = OW_IO;
      goto done;
    }
  }
  *out = s;
  return OW_OK;
done:
  ow_core_database_destroy(audited);
  if (m != INVALID_HANDLE_VALUE)
    CloseHandle(m);
  ow_store_close(s);
  return status;
}

OwStatus ow_store_open(const char *path,int create,OwStore **out,OwDatabase *db) {return ow_store_open_internal(path,create,out,db,0);}
