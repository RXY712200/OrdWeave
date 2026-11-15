/* Verified temporary-file publication adapted from Obscura64 replace_once,
 * 3db0122560853a8f3db3ba6888067394dc1a3e62, MIT (c) 2026 RXY712200.
 * Selection backup ordering and immutable generations are OrdWeave-specific. */
static void cp_filename(uint64_t gen, wchar_t out[80], int temp) {
  swprintf(out, 80, L"\\checkpoints\\%020llu.%ls", (unsigned long long)gen,
           temp ? L"pending" : L"cp");
}
static OwStatus cp_load_bytes(OwStore *s, const wchar_t *name, size_t limit,
                              unsigned char **out, size_t *size) {
  HANDLE h = open_file(s, name, GENERIC_READ, OPEN_EXISTING);
  uint64_t n;
  OwStatus status;
  *out = NULL;
  *size = 0;
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  if (!file_size(h, &n)) {
    CloseHandle(h);
    return OW_IO;
  }
  if (n > limit || n < 32) {
    CloseHandle(h);
    return OW_CORRUPT;
  }
  unsigned char *p = ow_checkpoint_oom() ? NULL : malloc((size_t)n);
  if (!p) {
    CloseHandle(h);
    return OW_NOMEM;
  }
  status = read_exact(h, p, (size_t)n);
  CloseHandle(h);
  if (status != OW_OK) {
    free(p);
    return status;
  }
  *out = p;
  *size = (size_t)n;
  return OW_OK;
}
static OwStatus cp_meta_validate(OwStore *s, const unsigned char *p, size_t n) {
  unsigned i, count, keep;
  uint64_t prev = UINT64_MAX, prevrev = UINT64_MAX;
  unsigned char zero[32] = {0};
  OwStatus status;
  if (n != OW_CHECKPOINT_META || memcmp(p, "OWSELECT", 8))
    return OW_CORRUPT;
  if (ow_get(p + 8, 4) != 1)
    return OW_UNSUPPORTED;
  if (ow_get(p + 12, 4) != OW_CHECKPOINT_META || memcmp(p + 16, s->id, 16) ||
      ow_get(p + 40, 8))
    return OW_CORRUPT;
  status = digest_check(p, n);
  if (status != OW_OK)
    return status;
  count = (unsigned)ow_get(p + 32, 4);
  keep = (unsigned)ow_get(p + 36, 4);
  if (!count || keep < 1 || keep > 3 || count > keep)
    return OW_CORRUPT;
  for (i = 0; i < 3; i++) {
    const unsigned char *e = p + 48 + 112 * i;
    if (i >= count) {
      unsigned j;
      for (j = 0; j < 112; j++)
        if (e[j])
          return OW_CORRUPT;
      continue;
    }
    uint64_t gen = ow_get(e, 8), rev = ow_get(e + 8, 8);
    if (!gen || (i && gen >= prev) || rev > prevrev || memcmp(e + 80, zero, 32))
      return OW_CORRUPT;
    prev = gen;
    prevrev = rev;
  }
  return OW_OK;
}
static OwStatus cp_verify_file(OwStore *s, const wchar_t *name,
                               const unsigned char *expected, size_t size,
                               int checkpoint) {
  unsigned char *p = NULL;
  size_t n = 0;
  OwDatabase *tmp = NULL;
  OwStatus status = cp_load_bytes(
      s, name, checkpoint ? OW_CHECKPOINT_LIMIT : OW_CHECKPOINT_META, &p, &n);
  if (status != OW_OK)
    return status;
  if (n != size || memcmp(p, expected, n))
    status = OW_CORRUPT;
  else if (checkpoint) {
    status = ow_core_database_create(&tmp);
    if (status == OW_OK)
      status = ow_core_import(tmp, p, n, s->id);
    ow_core_database_destroy(tmp);
  } else
    status = cp_meta_validate(s, p, n);
  free(p);
  return status;
}
static OwStatus cp_publish(OwStore *s, const wchar_t *temp,
                           const wchar_t *final, const unsigned char *p,
                           size_t n, int replace, int checkpoint) {
  wchar_t unique[128];
  if (!checkpoint) {
    unsigned char random[8];
    if (!BCRYPT_SUCCESS(
            BCryptGenRandom(NULL, random, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
      return OW_IO;
    swprintf(unique, 128, L"%ls.%016llx", temp,
             (unsigned long long)ow_get(random, 8));
    temp = unique;
  }
  HANDLE h = open_file(s, temp, GENERIC_WRITE, CREATE_NEW);
  OwStatus status;
  wchar_t *a = NULL, *b = NULL;
  int published = 0;
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  if(OW_FAULT(20)) {write_exact(h,p,n<17?n:17);status=OW_IO;}
  else status=write_exact(h,p,n);
  if (status == OW_OK && (OW_FAULT(21) || !FlushFileBuffers(h)))
    status = OW_IO;
  if (!CloseHandle(h) && status == OW_OK)
    status = OW_IO;
  if (status == OW_OK)
    status = cp_verify_file(s, temp, p, n, checkpoint);
  if (status == OW_OK) {
    a = child(s, temp);
    b = child(s, final);
    if (!a || !b)
      status = OW_NOMEM;
    else if (OW_FAULT(22))
      status = OW_IO;
    else if (!MoveFileExW(a, b,
                          MOVEFILE_WRITE_THROUGH |
                              (replace ? MOVEFILE_REPLACE_EXISTING : 0)))
      status = io_status();
    else {
      published = 1;
      if (OW_FAULT(23))
        status = OW_IO;
      else
        status = cp_verify_file(s, final, p, n, checkpoint);
    }
  }
  if (!published) {
    wchar_t *t = child(s, temp);
    if (t) {
      DeleteFileW(t);
      free(t);
    }
  }
  free(a);
  free(b);
  if (status != OW_OK && published && !checkpoint) {
    s->poisoned = 1;
    return OW_RECOVERY_REQUIRED;
  }
  return status;
}
static OwStatus cp_meta_read(OwStore *s, const wchar_t *name,
                             unsigned char out[OW_CHECKPOINT_META]) {
  unsigned char *p = NULL;
  size_t n = 0;
  OwStatus status = cp_load_bytes(s, name, OW_CHECKPOINT_META, &p, &n);
  if (status == OW_OK) {
    status = cp_meta_validate(s, p, n);
    if (status == OW_OK)
      memcpy(out, p, n);
  }
  free(p);
  return status;
}
/* Verify an anchor by rescanning authenticated-by-checksum WAL bytes, not by
 * trusting checkpoint metadata. Never reapply pre-checkpoint mutations. */
static OwStatus cp_anchor(OwStore *s, OwDatabase *db, uint64_t segments,
                          uint64_t revision, const unsigned char digest[32]) {
  uint64_t i;
  OwStatus status = OW_OK;
  if (revision > s->scan_revision)
    return OW_CORRUPT;
  if (s->wal != INVALID_HANDLE_VALUE) {
    CloseHandle(s->wal);
    s->wal = INVALID_HANDLE_VALUE;
  }
  s->checkpoint_revision = revision;
  memset(s->digest, 0, 32);
  memset(s->anchor_found, 0, 32);
  s->scan_revision = 0;
  s->validating = 1;
  for (i = 1;; i++) {
    status = replay_segment(s, db, i, i == segments);
    if (status != OW_OK)
      return status;
    if (i == segments)
      break;
  }
  return memcmp(s->anchor_found, digest, 32) ? OW_CORRUPT : OW_OK;
}
static OwStatus checkpoint_load(OwStore *s, OwDatabase *db, uint64_t segments) {
  wchar_t *directory = child(s, L"\\checkpoints");
  if (!directory)
    return OW_NOMEM;
  DWORD attributes = GetFileAttributesW(directory), error = GetLastError();
  free(directory);
  if (attributes != INVALID_FILE_ATTRIBUTES &&
      (!(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
       (attributes & FILE_ATTRIBUTE_REPARSE_POINT)))
    return OW_INVALID;
  if (attributes == INVALID_FILE_ATTRIBUTES && error != ERROR_FILE_NOT_FOUND &&
      error != ERROR_PATH_NOT_FOUND)
    return OW_IO;
  unsigned char current[OW_CHECKPOINT_META], backup[OW_CHECKPOINT_META];
  OwStatus a = cp_meta_read(s, L"\\checkpoint.current", current),
           b = cp_meta_read(s, L"\\checkpoint.current.bak", backup), status;
  unsigned char *meta = NULL;
  unsigned i, count;
  if (a == OW_UNSUPPORTED || b == OW_UNSUPPORTED)
    return OW_UNSUPPORTED;
  if ((a != OW_OK && a != OW_NOT_FOUND && a != OW_CORRUPT) ||
      (b != OW_OK && b != OW_NOT_FOUND && b != OW_CORRUPT))
    return a != OW_OK && a != OW_NOT_FOUND && a != OW_CORRUPT ? a : b;
  if (a == OW_OK && b == OW_OK) {
    uint64_t ag = ow_get(current + 48, 8), bg = ow_get(backup + 48, 8);
    if (ag == bg &&
        memcmp(current, backup,
               OW_CHECKPOINT_META)) { /* interrupted retention-only update:
                                         current is authoritative */
      if (memcmp(current + 48, backup + 48, 80))
        return OW_CORRUPT;
    }
    meta = ag >= bg ? current : backup;
  } else if (a == OW_OK)
    meta = current;
  else if (b == OW_OK)
    meta = backup;
  if (!meta) {
    s->checkpoint_revision = 0;
    return OW_OK;
  }
  memcpy(s->metadata, meta, OW_CHECKPOINT_META);
  s->retention = (unsigned)ow_get(meta + 36, 4);
  count = (unsigned)ow_get(meta + 32, 4);
  /* Validate both candidate metadata ancestries, including backup, against WAL.
   */
  for (i = 0; i < 2; i++) {
    unsigned char *m = i ? backup : current;
    OwStatus valid = i ? b : a;
    unsigned j;
    if (valid != OW_OK)
      continue;
    for (j = 0; j < (unsigned)ow_get(m + 32, 4); j++) {
      const unsigned char *e = m + 48 + 112 * j;
      status = cp_anchor(s, db, segments, ow_get(e + 8, 8), e + 48);
      if (status != OW_OK)
        return status;
    }
  }
  for (i = 0; i < count; i++) {
    const unsigned char *e = meta + 48 + 112 * i;
    wchar_t name[80];
    unsigned char *bytes = NULL;
    size_t n = 0;
    OwDatabase *tmp = NULL;
    cp_filename(ow_get(e, 8), name, 0);
    status = cp_load_bytes(s, name, OW_CHECKPOINT_LIMIT, &bytes, &n);
    if (status == OW_NOT_FOUND || status == OW_CORRUPT)
      continue;
    if (status != OW_OK)
      return status;
    if (n < 176 || memcmp(bytes + n - 32, e + 16, 32) ||
        ow_get(bytes + 40, 8) != ow_get(e, 8) ||
        ow_get(bytes + 48, 8) != ow_get(e + 8, 8) ||
        memcmp(bytes + 80, e + 48, 32)) {
      free(bytes);
      continue;
    }
    status = ow_core_database_create(&tmp);
    if (status == OW_OK)
      status = ow_core_import(tmp, bytes, n, s->id);
    free(bytes);
    if (status == OW_OK) {
      ow_core_exchange(db, tmp);
      ow_core_database_destroy(tmp);
      s->checkpoint_revision = ow_get(e + 8, 8);
      s->selected_generation = ow_get(e, 8);
      s->selected_revision = s->checkpoint_revision;
#ifdef OW_TESTING
      ++ow_test_checkpoint_restores;
#endif
      return OW_OK;
    }
    ow_core_database_destroy(tmp);
    if (status != OW_CORRUPT)
      return status;
  }
  s->checkpoint_revision = 0;
  return OW_OK; /* Full WAL already structurally validated; replay establishes
                   semantics. */
}
static OwStatus cp_directory(OwStore *s) {
  wchar_t *p = child(s, L"\\checkpoints");
  HANDLE h;
  BY_HANDLE_FILE_INFORMATION info;
  if (!p)
    return OW_NOMEM;
  if (!CreateDirectoryW(p, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
    free(p);
    return io_status();
  }
  h = CreateFileW(p, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, OPEN_EXISTING,
                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                  NULL);
  free(p);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  int valid = GetFileInformationByHandle(h, &info) &&
              (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
              !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
  CloseHandle(h);
  return valid ? OW_OK : OW_INVALID;
}
static OwStatus cp_next_generation(OwStore *s, uint64_t *out) {
  wchar_t *pattern = child(s, L"\\checkpoints\\*");
  WIN32_FIND_DATAW f;
  HANDLE h;
  uint64_t max = 0;
  OwStatus status = OW_OK;
  if (!pattern)
    return OW_NOMEM;
  h = FindFirstFileW(pattern, &f);
  free(pattern);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  do {
    size_t n = wcslen(f.cFileName), i;
    uint64_t gen = 0;
    if (!wcscmp(f.cFileName, L".") || !wcscmp(f.cFileName, L".."))
      continue;
    if ((n != 23 && n != 28) ||
        (f.dwFileAttributes &
         (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))) {
      status = OW_CORRUPT;
      break;
    }
    for (i = 0; i < 20; i++) {
      unsigned digit = (unsigned)(f.cFileName[i] - L'0');
      if (digit > 9 || gen > (UINT64_MAX - digit) / 10)
        break;
      gen = gen * 10 + digit;
    }
    if (i != 20 || !gen ||
        (wcscmp(f.cFileName + 20, L".cp") &&
         wcscmp(f.cFileName + 20, L".pending"))) {
      status = OW_CORRUPT;
      break;
    }
    if (gen > max)
      max = gen;
  } while (FindNextFileW(h, &f));
  if (status == OW_OK && GetLastError() != ERROR_NO_MORE_FILES)
    status = OW_IO;
  FindClose(h);
  if (status != OW_OK)
    return status;
  if (max < ow_get(s->metadata + 48, 8))
    max = ow_get(s->metadata + 48, 8);
  if (max == UINT64_MAX)
    return OW_CAPACITY;
  *out = max + 1;
  return OW_OK;
}
static OwStatus cp_cleanup(OwStore *s, const unsigned char *old) {
  unsigned i, n = (unsigned)ow_get(old + 32, 4), j,
              count = (unsigned)ow_get(s->metadata + 32, 4);
  for (i = 0; i < n; i++) {
    const unsigned char *e = old + 48 + 112 * i;
    int referenced = 0;
    wchar_t name[80];
    wchar_t *p;
    for (j = 0; j < count; j++)
      if (ow_get(e, 8) == ow_get(s->metadata + 48 + 112 * j, 8))
        referenced = 1;
    if (referenced)
      continue;
    if (OW_FAULT(27))
      return OW_IO;
    cp_filename(ow_get(e, 8), name, 0);
    p = child(s, name);
    if (!p)
      return OW_NOMEM;
    if (!DeleteFileW(p) && GetLastError() != ERROR_FILE_NOT_FOUND) {
      free(p);
      return io_status();
    }
    free(p);
  }
  return OW_OK;
}
/* Reclaim only independently validated, older, unreferenced generations after
 * current and backup have both been verified as the same selection. */
static OwStatus cp_sweep(OwStore *s) {
  unsigned char current[OW_CHECKPOINT_META], backup[OW_CHECKPOINT_META];
  OwStatus status = cp_meta_read(s, L"\\checkpoint.current", current);
  if (status != OW_OK)
    return status;
  status = cp_meta_read(s, L"\\checkpoint.current.bak", backup);
  if (status != OW_OK)
    return status;
  if (memcmp(current, backup, sizeof(current)))
    return OW_OK;
  wchar_t *pattern = child(s, L"\\checkpoints\\*.cp");
  WIN32_FIND_DATAW f;
  HANDLE h;
  if (!pattern)
    return OW_NOMEM;
  h = FindFirstFileW(pattern, &f);
  free(pattern);
  if (h == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND ? OW_OK : OW_IO;
  do {
    uint64_t gen = 0;
    size_t i;
    int referenced = 0;
    wchar_t name[80];
    unsigned char *bytes = NULL;
    size_t n = 0;
    OwDatabase *tmp = NULL;
    if (wcslen(f.cFileName) != 23 ||
        (f.dwFileAttributes &
         (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
      continue;
    for (i = 0; i < 20; i++) {
      unsigned digit = (unsigned)(f.cFileName[i] - L'0');
      if (digit > 9 || gen > (UINT64_MAX - digit) / 10)
        break;
      gen = gen * 10 + digit;
    }
    if (i != 20 || !gen || gen >= ow_get(current + 48, 8))
      continue;
    for (i = 0; i < (size_t)ow_get(current + 32, 4); i++)
      if (gen == ow_get(current + 48 + 112 * i, 8))
        referenced = 1;
    if (referenced)
      continue;
    cp_filename(gen, name, 0);
    status = cp_load_bytes(s, name, OW_CHECKPOINT_LIMIT, &bytes, &n);
    if (status == OW_OK) {
      status = ow_core_database_create(&tmp);
      if (status == OW_OK)
        status = ow_core_import(tmp, bytes, n, s->id);
    }
    free(bytes);
    ow_core_database_destroy(tmp);
    if (status == OW_CORRUPT || status == OW_UNSUPPORTED)
      continue;
    if (status != OW_OK) {
      FindClose(h);
      return status;
    }
    if (OW_FAULT(27)) {
      FindClose(h);
      return OW_IO;
    }
    wchar_t *p = child(s, name);
    if (!p) {
      FindClose(h);
      return OW_NOMEM;
    }
    if (!DeleteFileW(p)) {
      free(p);
      FindClose(h);
      return io_status();
    }
    free(p);
  } while (FindNextFileW(h, &f));
  DWORD error = GetLastError();
  FindClose(h);
  return error == ERROR_NO_MORE_FILES ? OW_OK : OW_IO;
}
OwStatus ow_checkpoint_info(const OwDatabase *db, uint64_t *generation,
                            uint64_t *revision) {
  OwStore *s = ow_core_store(db);
  if (generation)
    *generation = 0;
  if (revision)
    *revision = 0;
  if (!s || !generation || !revision)
    return OW_INVALID;
  OwStatus status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  *generation = s->selected_generation;
  *revision = s->selected_revision;
  return OW_OK;
}
OwStatus ow_checkpoint_retention(OwDatabase *db, unsigned n) {
  OwStore *s = ow_core_store(db);
  if (!s || n < 1 || n > 3)
    return OW_INVALID;
  OwStatus status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  s->retention = n;
  return OW_OK;
}
OwStatus ow_checkpoint_create(OwDatabase *db) {
  OwStore *s = ow_core_store(db);
  unsigned char *bytes = NULL, meta[OW_CHECKPOINT_META] = {0},
                old[OW_CHECKPOINT_META];
  size_t size = 0;
  uint64_t gen, rev;
  wchar_t temp[80], final[80];
  OwStatus status;
  unsigned count = 1, i;
  if (!s)
    return OW_INVALID;
  status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  ow_store_enter(s);
  status = cp_directory(s);
  if (status != OW_OK)
    goto done;
  status = cp_next_generation(s, &gen);
  if (status != OW_OK)
    goto done;
  ow_core_revision(db, &rev);
  status = ow_core_export(db, s->id, gen, s->digest, &bytes, &size);
  if (status != OW_OK)
    goto done;
  cp_filename(gen, temp, 1);
  cp_filename(gen, final, 0);
  status = cp_publish(s, temp, final, bytes, size, 0, 1);
  if (status != OW_OK)
    goto done;
  memcpy(old, s->metadata, sizeof(old));
  memcpy(meta, "OWSELECT", 8);
  ow_put(meta + 8, 1, 4);
  ow_put(meta + 12, sizeof(meta), 4);
  memcpy(meta + 16, s->id, 16);
  ow_put(meta + 36, s->retention, 4);
  ow_put(meta + 48, gen, 8);
  ow_put(meta + 56, rev, 8);
  memcpy(meta + 64, bytes + size - 32, 32);
  memcpy(meta + 96, s->digest, 32);
  for (i = 0; i < (unsigned)ow_get(old + 32, 4) && count < s->retention; i++) {
    const unsigned char *e = old + 48 + 112 * i;
    unsigned char *data = NULL;
    size_t n = 0;
    OwDatabase *tmp = NULL;
    wchar_t name[80];
    cp_filename(ow_get(e, 8), name, 0);
    status = cp_load_bytes(s, name, OW_CHECKPOINT_LIMIT, &data, &n);
    if (status == OW_NOT_FOUND || status == OW_CORRUPT)
      continue;
    if (status != OW_OK)
      goto done;
    if (n < 176 || memcmp(data + n - 32, e + 16, 32)) {
      free(data);
      continue;
    }
    status = ow_core_database_create(&tmp);
    if (status == OW_OK)
      status = ow_core_import(tmp, data, n, s->id);
    free(data);
    ow_core_database_destroy(tmp);
    if (status == OW_CORRUPT)
      continue;
    if (status != OW_OK)
      goto done;
    memcpy(meta + 48 + 112 * count++, e, 112);
  }
  ow_put(meta + 32, count, 4);
  status =
      ow_sha256(meta, sizeof(meta) - 32, NULL, 0, meta + sizeof(meta) - 32);
  if (status != OW_OK)
    goto done;
  /* Prior selection remains independently recoverable until new current
   * validates. */
  if (ow_get(old + 32, 4)) {
    status = cp_publish(s, L"\\checkpoint.backup.pending",
                        L"\\checkpoint.current.bak", old, sizeof(old), 1, 0);
    if (status != OW_OK)
      goto done;
  }
  if (OW_FAULT(24)) {
    status = OW_IO;
    goto done;
  }
  status = cp_publish(s, L"\\checkpoint.selection.pending",
                      L"\\checkpoint.current", meta, sizeof(meta), 1, 0);
  if (status != OW_OK)
    goto done;
  if (OW_FAULT(25)) {
    s->poisoned = 1;
    status = OW_RECOVERY_REQUIRED;
    goto done;
  }
  status = cp_publish(s, L"\\checkpoint.backup.pending",
                      L"\\checkpoint.current.bak", meta, sizeof(meta), 1, 0);
  if (status != OW_OK) {
    s->poisoned = 1;
    status = OW_RECOVERY_REQUIRED;
    goto done;
  }
  memcpy(s->metadata, meta, sizeof(meta));
  s->selected_generation = gen;
  s->selected_revision = rev;
  status = cp_cleanup(s, old);
  if (status == OW_OK)
    status = cp_sweep(s);
done:
  free(bytes);
  ow_store_leave(s);
  return status;
}
