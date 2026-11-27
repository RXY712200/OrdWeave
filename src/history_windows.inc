/* Versioned retained-history roots, immutable views, verified full-copy
 * backups. Uses the existing core, checkpoint codec and WAL parser; no second
 * model. */
#ifdef OW_TESTING
int ow_test_crash;
#define OW_AUDIT_POINT(n) do {if(ow_test_crash==(n)) TerminateProcess(GetCurrentProcess(),0);}while(0)
#else
#define OW_AUDIT_POINT(n) ((void)0)
#endif
struct OwHistory {
  OwDatabase *db;
};
static void history_scan_reset(OwStore *s) {
  s->scan_revision = s->earliest;
  memset(s->digest, 0, 32);
  memset(s->anchor_found, 0, 32);
  memset(s->root_anchor_found, 0, 32);
  if (s->recovery_root[0]) {
    memcpy(s->digest, s->recovery_root + 104, 32);
    if (s->earliest == ow_get(s->recovery_root + 152, 8))
      memcpy(s->root_anchor_found, s->digest, 32);
  }
}
static OwStatus history_root_validate(OwStore *s, const unsigned char *p,
                                      size_t n) {
  unsigned char zero[32] = {0};
  OwStatus status;
  if (n != 256 || memcmp(p, "OWROOT01", 8))
    return OW_CORRUPT;
  if (ow_get(p + 8, 4) != 1)
    return OW_UNSUPPORTED;
  status = digest_check(p, n);
  if (status != OW_OK)
    return status;
  if (ow_get(p + 12, 4) != 256 || memcmp(p + 16, s->id, 16) ||
      !ow_get(p + 32, 8) || !ow_get(p + 48, 8) || !ow_get(p + 88, 8) ||
      ow_get(p + 40, 8) == UINT64_MAX ||
      ow_get(p + 96, 8) != ow_get(p + 40, 8) + 1 ||
      ow_get(p + 136, 4) > OW_RETAIN_LAST_N || ow_get(p + 140, 4) ||
      ow_get(p + 152, 8) < ow_get(p + 40, 8) || memcmp(p + 192, zero, 32))
    return OW_CORRUPT;
  if ((ow_get(p + 136, 4) == OW_RETAIN_LAST_N) != (ow_get(p + 144, 8) != 0))
    return OW_CORRUPT;
  return OW_OK;
}
static OwStatus history_read_root(OwStore *s, const wchar_t *name,
                                  unsigned char out[256]) {
  unsigned char *p = NULL;
  size_t n = 0;
  OwStatus status = cp_load_bytes(s, name, 256, &p, &n);
  if (status == OW_OK) {
    status = history_root_validate(s, p, n);
    if (status == OW_OK)
      memcpy(out, p, 256);
  }
  free(p);
  return status;
}
static OwStatus history_policy_load(OwStore *s) {
  unsigned char *p = NULL;
  size_t n = 0;
  OwStatus status = cp_load_bytes(s, L"\\history.policy", 96, &p, &n);
  if (status == OW_NOT_FOUND)
    return OW_OK;
  if (status != OW_OK)
    return status;
  if (n != 96 || memcmp(p, "OWPOL001", 8))
    status = OW_CORRUPT;
  else if (ow_get(p + 8, 4) != 1)
    status = OW_UNSUPPORTED;
  else if ((status = digest_check(p, n)) == OW_OK) {
    unsigned i;
    if (ow_get(p + 12, 4) != 96 || memcmp(p + 16, s->id, 16) ||
        ow_get(p + 32, 4) > OW_RETAIN_LAST_N || ow_get(p + 36, 4))
      status = OW_CORRUPT;
    for (i = 48; i < 64; i++)
      if (p[i])
        status = OW_CORRUPT;
    if ((ow_get(p + 32, 4) == OW_RETAIN_LAST_N) != (ow_get(p + 40, 8) != 0))
      status = OW_CORRUPT;
    if (status == OW_OK) {
      s->history_policy = (OwRetentionPolicy)ow_get(p + 32, 4);
      s->history_n = ow_get(p + 40, 8);
    }
  }
  free(p);
  return status;
}
static OwStatus history_root_load(OwStore *s) {
  unsigned char a[256], b[256];
  OwStatus x = history_read_root(s, L"\\recovery.current", a),
           y = history_read_root(s, L"\\recovery.current.bak", b);
  s->first_segment = 1;
  s->earliest = 0;
  memset(s->recovery_root, 0, 256);
  if (x == OW_NOT_FOUND && y == OW_NOT_FOUND)
    return history_policy_load(s);
  if (x != OW_OK && x != OW_NOT_FOUND)
    return x;
  if (y != OW_OK && y != OW_NOT_FOUND)
    return y;
  const unsigned char *root = x == OW_OK ? a : b;
  if (x == OW_OK && y == OW_OK) {
    uint64_t ax = ow_get(a + 32, 8), bx = ow_get(b + 32, 8);
    if (ax == bx && memcmp(a, b, 256))
      return OW_CORRUPT;
    root = ax >= bx ? a : b;
    const unsigned char *old = ax >= bx ? b : a;
    if (ow_get(root + 40, 8) < ow_get(old + 40, 8) ||
        ow_get(root + 152, 8) < ow_get(old + 152, 8))
      return OW_CORRUPT;
  }
  memcpy(s->recovery_root, root, 256);
  s->first_segment = ow_get(root + 88, 8);
  s->earliest = ow_get(root + 40, 8);
  s->history_policy = (OwRetentionPolicy)ow_get(root + 136, 4);
  s->history_n = ow_get(root + 144, 8);
  return history_policy_load(s);
}
static OwStatus history_baseline(OwStore *s, OwDatabase *db) {
  if (!s->recovery_root[0])
    return OW_OK;
  if (s->scan_revision < ow_get(s->recovery_root + 152, 8) ||
      memcmp(s->root_anchor_found, s->recovery_root + 160, 32))
    return OW_CORRUPT;
  wchar_t name[80];
  unsigned char *p = NULL;
  size_t n = 0;
  OwStatus status;
  cp_filename(ow_get(s->recovery_root + 48, 8), name, 0);
  status = cp_load_bytes(s, name, OW_CHECKPOINT_LIMIT, &p, &n);
  if (status == OW_NOT_FOUND)
    return OW_RECOVERY_REQUIRED;
  if (status != OW_OK)
    return status;
  if (n < 176 || memcmp(p + n - 32, s->recovery_root + 56, 32) ||
      ow_get(p + 40, 8) != ow_get(s->recovery_root + 48, 8) ||
      ow_get(p + 48, 8) != s->earliest ||
      memcmp(p + 80, s->recovery_root + 104, 32))
    status = OW_CORRUPT;
  else
    status = ow_core_import(db, p, n, s->id);
  free(p);
  if (status == OW_OK) {
    s->checkpoint_revision = s->earliest;
    s->selected_generation = ow_get(s->recovery_root + 48, 8);
    s->selected_revision = s->earliest;
  }
  return status;
}
static void history_scan_close(OwStore *s) {
  if (s->wal != INVALID_HANDLE_VALUE) {
    CloseHandle(s->wal);
    s->wal = INVALID_HANDLE_VALUE;
  }
}
static OwStatus history_same_state(OwDatabase *a, OwDatabase *b,
                                   const unsigned char id[16]);
/* Byte integrity is not semantic validity. Audit every retained operation from
 * the authoritative baseline independently of checkpoint acceleration. */
static OwStatus history_semantic_state(OwStore *source, uint64_t target,
                                       OwDatabase **out) {
  OwStore scan = *source;
  OwDatabase *db = NULL;
  uint64_t count, i, latest = source->scan_revision;
  OwStatus status;
  *out = NULL;
  scan.wal = INVALID_HANDLE_VALUE;
  scan.readonly_scan = 1;
  scan.semantic_scan = 1;
  scan.allow_incomplete_tail = !source->readonly_scan;
  scan.validating = 0;
  scan.limit_replay = 0;
  scan.checkpoint_revision = source->earliest;
  status = discover(&scan, &count);
  if (status != OW_OK)
    return status;
  for (unsigned pass = 0; pass < (target < latest ? 2u : 1u); pass++) {
    status = ow_core_database_create(&db);
    if (status != OW_OK)
      goto done;
    /* source still carries the structurally verified publication anchor. */
    scan.scan_revision = source->scan_revision;
    memcpy(scan.root_anchor_found, source->root_anchor_found, 32);
    status = history_baseline(&scan, db);
    if (status != OW_OK)
      goto done;
    scan.limit_replay = pass != 0;
    scan.through = target;
    scan.checkpoint_revision = source->earliest;
    history_scan_reset(&scan);
    for (i = scan.first_segment;; i++) {
      status = replay_segment(&scan, db, i, i == count);
      if (status != OW_OK)
        goto done;
      if (i == count)
        break;
    }
    history_scan_close(&scan);
    if (target < latest && !pass) {
      ow_core_database_destroy(db);
      db = NULL;
    }
  }
  *out = db;
  db = NULL;
  status = OW_OK;
done:
  history_scan_close(&scan);
  ow_core_database_destroy(db);
  return status;
}
static OwStatus history_reconstruct_impl(OwStore *live, uint64_t target,
                                         OwDatabase **out, OwStore *state,
                                         int proposed) {
  OwStore scan = *live;
  OwDatabase *db = NULL, *audited = NULL;
  uint64_t count, i, latest = 0;
  OwStatus status;
  *out = NULL;
  scan.wal = INVALID_HANDLE_VALUE;
  scan.readonly_scan = 1;
  scan.validating = 1;
  scan.limit_replay = 1;
  scan.through = target;
  if (ow_checkpoint_oom())
    return OW_NOMEM;
  status = ow_core_database_create(&db);
  if (status != OW_OK)
    return status;
  /* Reread recovery metadata and manifest, not only the live state. */
  unsigned char *manifest = NULL;
  size_t n = 0;
  status = cp_load_bytes(&scan, L"\\manifest.ow", 96, &manifest, &n);
  if (status == OW_OK && (n != 96 || memcmp(manifest, "OWMAN003", 8) ||
                          memcmp(manifest + 16, live->id, 16)))
    status = OW_CORRUPT;
  if (status == OW_OK && ow_get(manifest + 8, 4) != 3)
    status = OW_UNSUPPORTED;
  if (status == OW_OK) {
    unsigned i;
    if (ow_get(manifest + 12, 4) != 96 ||
        ow_get(manifest + 32, 8) != OW_SEGMENT_TARGET ||
        ow_get(manifest + 40, 8) != OW_FRAME_LIMIT)
      status = OW_UNSUPPORTED;
    for (i = 48; i < 64; i++)
      if (manifest[i])
        status = OW_UNSUPPORTED;
  }
  if (status == OW_OK)
    status = digest_check(manifest, n);
  free(manifest);
  if (status != OW_OK)
    goto done;
  if (!proposed) {
    status = history_root_load(&scan);
    if (status != OW_OK)
      goto done;
  }
  if (target < scan.earliest) {
    status = OW_HISTORY_UNAVAILABLE;
    goto done;
  }
  status = discover(&scan, &count);
  if (status != OW_OK)
    goto done;
  history_scan_reset(&scan);
  for (i = scan.first_segment;; i++) {
    status = replay_segment(&scan, db, i, i == count);
    if (status != OW_OK)
      goto done;
    if (i == count)
      break;
  }
  latest = scan.scan_revision;
  if (target > latest) {
    status = OW_HISTORY_UNAVAILABLE;
    goto done;
  }
  status=history_semantic_state(&scan,target,&audited);if(status!=OW_OK) goto done;
  status = history_baseline(&scan, db);
  if (status != OW_OK)
    goto done;
  status = checkpoint_load(&scan, db, count);
  if (status != OW_OK)
    goto done;
  history_scan_close(&scan);
  history_scan_reset(&scan);
  scan.validating = 0;
  for (i = scan.first_segment;; i++) {
    status = replay_segment(&scan, db, i, i == count);
    if (status != OW_OK)
      goto done;
    if (i == count)
      break;
  }
  {
    uint64_t r = 0;
    ow_core_revision(db, &r);
    if (r != target) {
      status = OW_CORRUPT;
      goto done;
    }
  }
  status=history_same_state(db,audited,scan.id);if(status!=OW_OK) goto done;
  if (state) {
    *state = scan;
    state->wal = INVALID_HANDLE_VALUE;
  }
  *out = db;
  db = NULL;
  status = OW_OK;
done:
  history_scan_close(&scan);
  ow_core_database_destroy(audited);
  ow_core_database_destroy(db);
  return status;
}
static OwStatus history_reconstruct(OwStore *live, uint64_t target,
                                    OwDatabase **out, OwStore *state) {
  return history_reconstruct_impl(live, target, out, state, 0);
}
OwStatus ow_history_open(OwDatabase *db, uint64_t revision, OwHistory **out) {
  OwStore *s = ow_core_store(db);
  OwStatus status;
  OwDatabase *copy = NULL;
  if (out)
    *out = NULL;
  if (!s || !out)
    return OW_INVALID;
  status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  status = history_reconstruct(s, revision, &copy, NULL);
  if (status != OW_OK)
    return status;
  OwHistory *view = ow_checkpoint_oom() ? NULL : malloc(sizeof(*view));
  if (!view) {
    ow_core_database_destroy(copy);
    return OW_NOMEM;
  }
  view->db = copy;
  *out = view;
  return OW_OK;
}
void ow_history_close(OwHistory *view) {
  if (view) {
    ow_core_database_destroy(view->db);
    free(view);
  }
}
/* No API accepts a historical view for mutation or reveals its core pointer. */
OwStatus ow_history_revision(const OwHistory *v, uint64_t *out) {
  return ow_core_revision(v ? v->db : NULL, out);
}
OwStatus ow_history_record_exists(const OwHistory *v, OwRecordId id, int *out) {
  return ow_core_record_exists(v ? v->db : NULL, id, out);
}
OwStatus ow_history_field_get(const OwHistory *v, OwRecordId id,
                              const char *name, OwValue *out) {
  return ow_core_field_get(v ? v->db : NULL, id, name, out);
}
OwStatus ow_history_collection_count(const OwHistory *v, OwCollectionId id,
                                     size_t *out) {
  return ow_core_collection_count(v ? v->db : NULL, id, out);
}
OwStatus ow_history_collection_members(const OwHistory *v, OwCollectionId id,
                                       OwOccurrenceId **out, size_t *count) {
  return ow_core_collection_members(v ? v->db : NULL, id, out, count);
}
OwStatus ow_history_occurrence_get(const OwHistory *v, OwOccurrenceId id,
                                   OwCollectionId *c, OwRecordId *r) {
  return ow_core_occurrence_get(v ? v->db : NULL, id, c, r);
}
OwStatus ow_history_occurrence_compare(const OwHistory *v, OwOccurrenceId a,
                                       OwOccurrenceId b, int *out) {
  return ow_core_occurrence_compare(v ? v->db : NULL, a, b, out);
}
OwStatus ow_history_query_equal(const OwHistory *v, const char *name,
                                const OwValue *value, OwRecordId **out,
                                size_t *count) {
  return ow_core_query_equal(v ? v->db : NULL, name, value, out, count);
}
static OwStatus history_publish(OwStore *s, const wchar_t *name,
                                const unsigned char *bytes, size_t n,
                                int fault) {
  (void)fault;
  wchar_t temp[128];
  unsigned char random[8];
  HANDLE h;
  OwStatus status;
  wchar_t *a = NULL, *b = NULL;
  int published = 0;
  if (!BCRYPT_SUCCESS(
          BCryptGenRandom(NULL, random, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
    return OW_IO;
  swprintf(temp, 128, L"%ls.%016llx.pending", name,
           (unsigned long long)ow_get(random, 8));
  h = open_file(s, temp, GENERIC_WRITE, CREATE_NEW);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  status = OW_FAULT(fault) ? OW_IO : write_exact(h, bytes, n);
  if (status == OW_OK && (OW_FAULT(fault + 1) || !FlushFileBuffers(h)))
    status = OW_IO;
  if (!CloseHandle(h) && status == OW_OK)
    status = OW_IO;
  if (status == OW_OK) {
    unsigned char *p = NULL;
    size_t size = 0;
    status = cp_load_bytes(s, temp, n, &p, &size);
    if (status == OW_OK && (size != n || memcmp(p, bytes, n)))
      status = OW_CORRUPT;
    free(p);
  }
  if (status == OW_OK) {
    a = child(s, temp);
    b = child(s, name);
    if (!a || !b)
      status = OW_NOMEM;
    else if (OW_FAULT(fault + 2) ||
             !MoveFileExW(a, b,
                          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
      status = OW_IO;
    else
      published = 1;
  }
  if (status == OW_OK && published) {
    unsigned char *p = NULL;
    size_t size = 0;
    status = cp_load_bytes(s, name, n, &p, &size);
    if (status == OW_OK && (size != n || memcmp(p, bytes, n)))
      status = OW_CORRUPT;
    free(p);
    if (OW_FAULT(fault + 3))
      status = OW_IO;
  }
  if (!published) {
    wchar_t *p = child(s, temp);
    if (p) {
      DeleteFileW(p);
      free(p);
    }
  }
  free(a);
  free(b);
  if (status != OW_OK && published) {
    s->poisoned = 1;
    return OW_RECOVERY_REQUIRED;
  }
  return status;
}
OwStatus ow_history_policy(OwDatabase *db, OwRetentionPolicy policy,
                           uint64_t n) {
  OwStore *s = ow_core_store(db);
  unsigned char p[96] = {0};
  OwStatus status;
  if (!s || policy < OW_RETAIN_MANUAL || policy > OW_RETAIN_LAST_N ||
      ((policy == OW_RETAIN_LAST_N) != (n != 0)))
    return OW_INVALID;
  status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  if (policy == OW_RETAIN_LAST_N) {
    uint64_t current;
    ow_core_revision(db, &current);
    uint64_t floor = current >= n ? current - n + 1 : 0;
    if (floor < s->earliest)
      return OW_HISTORY_UNAVAILABLE;
  }
  memcpy(p, "OWPOL001", 8);
  ow_put(p + 8, 1, 4);
  ow_put(p + 12, 96, 4);
  memcpy(p + 16, s->id, 16);
  ow_put(p + 32, policy, 4);
  ow_put(p + 40, n, 8);
  status = ow_sha256(p, 64, NULL, 0, p + 64);
  if (status != OW_OK)
    return status;
  status = history_publish(s, L"\\history.policy", p, 96, 60);
  if (status == OW_OK) {
    s->history_policy = policy;
    s->history_n = n;
  }
  return status;
}
/* Find the latest complete segment boundary at or before the promised floor. */
static OwStatus history_boundary(OwStore *s, uint64_t floor, uint64_t *sequence,
                                 uint64_t *revision, unsigned char digest[32]) {
  uint64_t count, i;
  OwStatus status = discover(s, &count);
  if (status != OW_OK)
    return status;
  *sequence = s->first_segment;
  *revision = s->earliest;
  memset(digest, 0, 32);
  if (s->recovery_root[0])
    memcpy(digest, s->recovery_root + 104, 32);
  for (i = s->first_segment;; i++) {
    wchar_t name[64];
    unsigned char header[128];
    HANDLE h;
    segment_name(i, name, 0);
    h = open_file(s, name, GENERIC_READ, OPEN_EXISTING);
    if (h == INVALID_HANDLE_VALUE)
      return io_status();
    status = read_exact(h, header, 128);
    CloseHandle(h);
    if (status != OW_OK)
      return status;
    status = digest_check(header, 128);
    if (status != OW_OK)
      return status;
    uint64_t start = ow_get(header + 40, 8);
    if (start && start - 1 <= floor) {
      *sequence = i;
      *revision = start - 1;
      memcpy(digest, header + 48, 32);
    }
    if (i == count)
      break;
  }
  return OW_OK;
}
OwStatus ow_history_range(OwDatabase *db, OwHistoryRange *out) {
  OwStore *s = ow_core_store(db);
  OwStatus status;
  if (out)
    memset(out, 0, sizeof(*out));
  if (!s || !out)
    return OW_INVALID;
  status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  ow_core_revision(db, &out->current_revision);
  OwDatabase *oldest = NULL;
  status = history_reconstruct(s, s->earliest, &oldest, NULL);
  ow_core_database_destroy(oldest);
  if (status != OW_OK) {
    memset(out, 0, sizeof(*out));
    return status;
  }
  out->earliest_revision = s->earliest;
  out->policy = s->history_policy;
  out->last_n = s->history_n;
  if (s->history_policy == OW_RETAIN_LAST_N) {
    uint64_t floor = out->current_revision >= s->history_n
                         ? out->current_revision - s->history_n + 1
                         : 0,
             seq, rev;
    unsigned char digest[32];
    status = history_boundary(s, floor, &seq, &rev, digest);
    if (status != OW_OK)
      return status;
    out->reclaimable = seq > s->first_segment && s->retention >= 2;
  }
  return OW_OK;
}
/* Verification is a read-only reconstruction; never tail truncation or
 * deletion. */
OwStatus ow_verify_database(OwDatabase *db, OwHistoryRange *out) {
  OwStore *s = ow_core_store(db);
  OwDatabase *copy = NULL;
  OwStatus status;
  uint64_t revision;
  if (out)
    memset(out, 0, sizeof(*out));
  if (!s || !out)
    return OW_INVALID;
  status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  ow_core_revision(db, &revision);
  if (s->recovery_root[0]) {
    unsigned char a[256], b[256];
    status = history_read_root(s, L"\\recovery.current", a);
    if (status != OW_OK)
      return status;
    status = history_read_root(s, L"\\recovery.current.bak", b);
    if (status != OW_OK)
      return status;
    if (memcmp(a, b, 256))
      return OW_RECOVERY_REQUIRED;
  }
  status = history_reconstruct(s, revision, &copy, NULL);
  ow_core_database_destroy(copy);
  if (status != OW_OK)
    return status;
  unsigned char m[OW_CHECKPOINT_META], backup_meta[OW_CHECKPOINT_META];
  OwStatus backup_status =
      cp_meta_read(s, L"\\checkpoint.current.bak", backup_meta);
  if (backup_status != OW_OK && backup_status != OW_NOT_FOUND)
    return backup_status;
  status = cp_meta_read(s, L"\\checkpoint.current", m);
  if (status != OW_NOT_FOUND && status != OW_OK)
    return status;
  if ((status == OW_OK) != (backup_status == OW_OK))
    return OW_RECOVERY_REQUIRED;
  if (status == OW_OK && memcmp(m, backup_meta, sizeof(m)))
    return OW_RECOVERY_REQUIRED;
  if (status == OW_OK) {
    unsigned i;
    for (i = 0; i < (unsigned)ow_get(m + 32, 4); i++) {
      const unsigned char *e = m + 48 + 112 * i;
      wchar_t name[80];
      unsigned char *p = NULL;
      size_t n = 0;
      cp_filename(ow_get(e, 8), name, 0);
      status = cp_load_bytes(s, name, OW_CHECKPOINT_LIMIT, &p, &n);
      if (status != OW_OK)
        return status;
      if (n < 176 || memcmp(p + n - 32, e + 16, 32))
        status = OW_CORRUPT;
      else {
        status = ow_core_database_create(&copy);
        if (status == OW_OK)
          status = ow_core_import(copy, p, n, s->id);
      }
      free(p);
      ow_core_database_destroy(copy);
      copy = NULL;
      if (status != OW_OK)
        return status;
    }
  }
  return ow_history_range(db, out);
}
static OwStatus history_same_state(OwDatabase *a, OwDatabase *b,
                                   const unsigned char id[16]) {
  (void)id;
  return ow_core_same_state(a, b) ? OW_OK : OW_CORRUPT;
}
static OwStatus history_selection(OwStore *s, const unsigned char *checkpoint,
                                  size_t size) {
  unsigned char m[OW_CHECKPOINT_META] = {0};
  OwStatus status;
  memcpy(m, "OWSELECT", 8);
  ow_put(m + 8, 1, 4);
  ow_put(m + 12, sizeof(m), 4);
  memcpy(m + 16, s->id, 16);
  ow_put(m + 32, 1, 4);
  ow_put(m + 36, s->retention, 4);
  memcpy(m + 48, checkpoint + 40, 16);
  memcpy(m + 64, checkpoint + size - 32, 32);
  memcpy(m + 96, checkpoint + 80, 32);
  status = ow_sha256(m, sizeof(m) - 32, NULL, 0, m + sizeof(m) - 32);
  if (status != OW_OK)
    return status;
  status = cp_publish(s, L"\\history.selection.pending",
                      L"\\checkpoint.current", m, sizeof(m), 1, 0);
  if (status != OW_OK)
    return status;
  status = cp_publish(s, L"\\history.backup.pending",
                      L"\\checkpoint.current.bak", m, sizeof(m), 1, 0);
  if (status != OW_OK) {
    s->poisoned = 1;
    return OW_RECOVERY_REQUIRED;
  }
  memcpy(s->metadata, m, sizeof(m));
  s->selected_generation = ow_get(checkpoint + 40, 8);
  s->selected_revision = ow_get(checkpoint + 48, 8);
  return OW_OK;
}
static OwStatus history_delete_prefix(OwStore *s, uint64_t oldfirst) {
  unsigned char a[256], b[256];
  OwStatus status = history_read_root(s, L"\\recovery.current", a);
  if (status != OW_OK)
    return status;
  status = history_read_root(s, L"\\recovery.current.bak", b);
  if (status != OW_OK)
    return status;
  if (memcmp(a, b, 256) || memcmp(a, s->recovery_root, 256))
    return OW_RECOVERY_REQUIRED;
  for (uint64_t i = oldfirst; i < s->first_segment; i++) {
    wchar_t name[64], *p;
    segment_name(i, name, 0);
    p = child(s, name);
    if (!p)
      return OW_NOMEM;
    if (OW_FAULT(48)) {
      free(p);
      return OW_IO;
    }
    if (!DeleteFileW(p) && GetLastError() != ERROR_FILE_NOT_FOUND) {
      free(p);
      return io_status();
    }
    free(p);
    if(i==oldfirst) OW_AUDIT_POINT(73);
  }
  return OW_OK;
}
OwStatus ow_history_cleanup(OwDatabase *db, OwHistoryRange *out) {
  OwStore *s = ow_core_store(db);
  OwStatus status;
  OwHistoryRange range;
  OwDatabase *baseline = NULL, *check = NULL;
  unsigned char digest[32], *bytes = NULL, root[256] = {0},
                            oldmeta[OW_CHECKPOINT_META];
  size_t size = 0;
  uint64_t floor, seq, revision, gen, latest, oldfirst, epoch;
  wchar_t temp[80], final[80];
  if (out)
    memset(out, 0, sizeof(*out));
  if (!s || !out)
    return OW_INVALID;
  /* Quarantined or busy sessions must not reconstruct or repair disk roots. */
  status = ow_store_guard(s);
  if (status != OW_OK)
    return status;
  if (s->recovery_root[0]) {
    OwDatabase *validated = NULL;
    uint64_t now;
    ow_core_revision(db, &now);
    status = history_reconstruct(s, now, &validated, NULL);
    if (status == OW_OK)
      status = history_same_state(db, validated, s->id);
    ow_core_database_destroy(validated);
    if (status != OW_OK)
      return status;
    unsigned char a[256], b[256];
    OwStatus x = history_read_root(s, L"\\recovery.current", a),
             y = history_read_root(s, L"\\recovery.current.bak", b);
    if (x != OW_OK || memcmp(a, s->recovery_root, 256)) {
      status =
          history_publish(s, L"\\recovery.current", s->recovery_root, 256, 40);
      if (status != OW_OK)
        return status;
    }
    if (y != OW_OK || memcmp(b, s->recovery_root, 256)) {
      status = history_publish(s, L"\\recovery.current.bak", s->recovery_root,
                               256, 44);
      if (status != OW_OK)
        return status;
    }
  }
  status = ow_verify_database(db, &range);
  if (status != OW_OK)
    return status;
  if (s->history_policy != OW_RETAIN_LAST_N) {
    if (s->recovery_root[0]) {
      status = history_delete_prefix(s, 1);
      if (status != OW_OK)
        return status;
    }
    *out = range;
    return OW_OK;
  }
  latest = range.current_revision;
  floor = latest >= s->history_n ? latest - s->history_n + 1 : 0;
  status = history_boundary(s, floor, &seq, &revision, digest);
  if (status != OW_OK)
    return status;
  if (seq == s->first_segment) {
    if (s->recovery_root[0]) {
      status = history_delete_prefix(s, 1);
      if (status != OW_OK)
        return status;
    }
    return ow_history_range(db, out);
  }
  if (s->retention < 2)
    return OW_RETENTION_UNSUPPORTED;
  oldfirst = s->first_segment;
  epoch = s->recovery_root[0] ? ow_get(s->recovery_root + 32, 8) : 0;
  if (epoch == UINT64_MAX)
    return OW_CAPACITY;
  status = history_reconstruct(s, revision, &baseline, NULL);
  if (status != OW_OK)
    goto done;
  status = cp_directory(s);
  if (status != OW_OK)
    goto done;
  status = cp_next_generation(s, &gen);
  if (status != OW_OK)
    goto done;
  status = ow_core_export(baseline, s->id, gen, digest, &bytes, &size);
  if (status != OW_OK)
    goto done;
  cp_filename(gen, temp, 1);
  cp_filename(gen, final, 0);
  status = cp_publish(s, temp, final, bytes, size, 0, 1);
  if (status != OW_OK)
    goto done;
  memcpy(root, "OWROOT01", 8);
  ow_put(root + 8, 1, 4);
  ow_put(root + 12, 256, 4);
  memcpy(root + 16, s->id, 16);
  ow_put(root + 32, epoch + 1, 8);
  ow_put(root + 40, revision, 8);
  ow_put(root + 48, gen, 8);
  memcpy(root + 56, bytes + size - 32, 32);
  ow_put(root + 88, seq, 8);
  ow_put(root + 96, revision + 1, 8);
  memcpy(root + 104, digest, 32);
  ow_put(root + 136, s->history_policy, 4);
  ow_put(root + 144, s->history_n, 8);
  ow_put(root + 152, latest, 8);
  memcpy(root + 160, s->digest, 32);
  status = ow_sha256(root, 224, NULL, 0, root + 224);
  if (status != OW_OK)
    goto done;
  {
    OwStore proposed = *s;
    memcpy(proposed.recovery_root, root, 256);
    proposed.first_segment = seq;
    proposed.earliest = revision;
    status = history_reconstruct_impl(&proposed, latest, &check, NULL, 1);
    if (status == OW_OK)
      status = history_same_state(db, check, s->id);
    ow_core_database_destroy(check);
    check = NULL;
    if (status != OW_OK)
      goto done;
  }
  memcpy(oldmeta, s->metadata, sizeof(oldmeta));
  status = history_selection(s, bytes, size);
  if (status != OW_OK)
    goto done;
  OW_AUDIT_POINT(70);
  status = history_publish(s, L"\\recovery.current", root, 256, 40);
  if (status != OW_OK)
    goto done;
  OW_AUDIT_POINT(71);
  if (OW_FAULT(44)) {
    s->poisoned = 1;
    status = OW_RECOVERY_REQUIRED;
    goto done;
  }
  status = history_publish(s, L"\\recovery.current.bak", root, 256, 44);
  if (status != OW_OK) {
    s->poisoned = 1;
    status = OW_RECOVERY_REQUIRED;
    goto done;
  }
  memcpy(s->recovery_root, root, 256);
  s->first_segment = seq;
  s->earliest = revision;
  s->selected_generation = gen;
  s->selected_revision = revision;
  status = history_reconstruct(s, latest, &check, NULL);
  if (status == OW_OK)
    status = history_same_state(db, check, s->id);
  ow_core_database_destroy(check);
  check = NULL;
  if (status != OW_OK) {
    s->poisoned = 1;
    status = OW_RECOVERY_REQUIRED;
    goto done;
  }
  OW_AUDIT_POINT(72);
  status = history_delete_prefix(s, oldfirst);
  if (status != OW_OK)
    goto done;
  OW_AUDIT_POINT(74);
  status = cp_cleanup(s, oldmeta);
  if (status == OW_OK)
    status = cp_sweep(s);
  if (status == OW_OK)
    status = ow_verify_database(db, out);
done:
  ow_core_database_destroy(check);
  ow_core_database_destroy(baseline);
  free(bytes);
  return status;
}
/* Backups are ordinary directories with identical immutable format bytes. A
 * completion marker is published last; manifest is withheld until all copies
 * are flushed, so interrupted copies are never silently opened as complete. */
static OwStatus history_copy(OwStore *source, OwStore *dest,
                             const wchar_t *name) {
  HANDLE in = open_file(source, name, GENERIC_READ, OPEN_EXISTING),
         out = INVALID_HANDLE_VALUE;
  OwStatus status;
  unsigned char *buffer = NULL;
  if (in == INVALID_HANDLE_VALUE)
    return io_status();
  out = open_file(dest, name, GENERIC_WRITE, CREATE_NEW);
  if (out == INVALID_HANDLE_VALUE) {
    CloseHandle(in);
    return io_status();
  }
  buffer = ow_checkpoint_oom() ? NULL : malloc(65536);
  status = buffer ? OW_OK : OW_NOMEM;
  while (status == OW_OK) {
    DWORD n = 0;
    if (!ReadFile(in, buffer, 65536, &n, NULL)) {
      status = OW_IO;
      break;
    }
    if (!n)
      break;
    if (OW_FAULT(50)) {
      status = OW_IO;
      break;
    }
    status = write_exact(out, buffer, n);
  }
  if (status == OW_OK && (OW_FAULT(51) || !FlushFileBuffers(out)))
    status = OW_IO;
  free(buffer);
  CloseHandle(in);
  if (!CloseHandle(out) && status == OW_OK)
    status = OW_IO;
  return status;
}
static OwStatus history_copy_directory(OwStore *source, OwStore *dest,
                                       const wchar_t *directory) {
  wchar_t pattern[100], suffix[160];
  swprintf(pattern, 100, L"%ls\\*", directory);
  wchar_t *path = child(source, pattern);
  WIN32_FIND_DATAW f;
  HANDLE find;
  OwStatus status = OW_OK;
  if (!path)
    return OW_NOMEM;
  find = FindFirstFileW(path, &f);
  free(path);
  if (find == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_PATH_NOT_FOUND ? OW_OK : io_status();
  do {
    size_t n = wcslen(f.cFileName);
    if (!wcscmp(f.cFileName, L".") || !wcscmp(f.cFileName, L".."))
      continue;
    if (f.dwFileAttributes &
        (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) {
      status = OW_CORRUPT;
      break;
    }
    /* Never format unchecked directory names into a bounded path buffer. */
    int wal=!wcscmp(directory,L"\\wal");size_t j;uint64_t sequence=0;
    if(n!=28 && n!=(wal?24u:23u)) {status=OW_CORRUPT;break;}
    for(j=0;j<20;j++) {unsigned digit=(unsigned)(f.cFileName[j]-L'0');if(digit>9 || sequence>(UINT64_MAX-digit)/10) break;sequence=sequence*10+digit;}
    if(j!=20 || !sequence) {status=OW_CORRUPT;break;}
    if(n==28) {if(wcscmp(f.cFileName+20,L".pending")) {status=OW_CORRUPT;break;}continue;}
    if(wcscmp(f.cFileName+20,wal?L".wal":L".cp")) {status=OW_CORRUPT;break;}
    int written=swprintf(suffix,160,L"%ls\\%ls",directory,f.cFileName);
    if(written<0 || written>=160) {status=OW_CAPACITY;break;}
    status = history_copy(source, dest, suffix);
    if (status != OW_OK)
      break;
  } while (FindNextFileW(find, &f));
  if (status == OW_OK && GetLastError() != ERROR_NO_MORE_FILES)
    status = OW_IO;
  FindClose(find);
  return status;
}
static OwStatus history_backup_parent(OwStore *source,
                                      const wchar_t *destination) {
  wchar_t parent[260], canonical_parent[1024], canonical_source[1024];
  size_t n = wcslen(destination);
  if (n >= 260)
    return OW_INVALID;
  memcpy(parent, destination, (n + 1) * sizeof(wchar_t));
  wchar_t *last = wcsrchr(parent, L'\\');
  if (!last)
    return OW_INVALID;
  if (last == parent + 2)
    last[1] = 0;
  else
    *last = 0;
  HANDLE h = CreateFileW(parent, FILE_READ_ATTRIBUTES,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (h == INVALID_HANDLE_VALUE)
    return io_status();
  DWORD a = GetFinalPathNameByHandleW(h, canonical_parent, 1024,
                                      FILE_NAME_NORMALIZED | VOLUME_NAME_GUID);
  DWORD b = GetFinalPathNameByHandleW(source->root, canonical_source, 1024,
                                      FILE_NAME_NORMALIZED | VOLUME_NAME_GUID);
  CloseHandle(h);
  if (!a || !b || a >= 1024 || b >= 1024)
    return OW_IO;
  if (a >= b && !_wcsnicmp(canonical_parent, canonical_source, b) &&
      (canonical_parent[b] == 0 || canonical_parent[b] == L'\\'))
    return OW_INVALID;
  return OW_OK;
}
OwStatus ow_backup_create(OwDatabase *db, const char *destination) {
  OwStore *s = ow_core_store(db), dest = {0};
  OwStatus status;
  OwHistoryRange range;
  wchar_t *path = NULL;
  HANDLE lock = INVALID_HANDLE_VALUE;
  OwDatabase *copy = NULL;
  OwStore *opened = NULL;
  if (!s)
    return OW_INVALID;
  status = ow_verify_database(db, &range);
  if (status != OW_OK)
    return status;
  status = safe_path(destination, &path);
  if (status != OW_OK)
    return status;
  size_t n = wcslen(s->path);
  if (!_wcsnicmp(path, s->path, n) && (path[n] == 0 || path[n] == L'\\')) {
    free(path);
    return OW_INVALID;
  }
  status = history_backup_parent(s, path);
  if (status != OW_OK) {
    free(path);
    return status;
  }
  if (!CreateDirectoryW(path, NULL)) {
    free(path);
    return GetLastError() == ERROR_ALREADY_EXISTS ? OW_CONFLICT : io_status();
  }
  dest.path = path;
  dest.wal = dest.root = dest.directory = dest.lock = INVALID_HANDLE_VALUE;
  wchar_t *p = child(&dest, L"\\wal");
  if (!p) {
    status = OW_NOMEM;
    goto done;
  }
  if (!CreateDirectoryW(p, NULL)) {
    free(p);
    status = OW_IO;
    goto done;
  }
  free(p);
  p = child(&dest, L"\\checkpoints");
  if (!p) {
    status = OW_NOMEM;
    goto done;
  }
  if (!CreateDirectoryW(p, NULL)) {
    free(p);
    status = OW_IO;
    goto done;
  }
  free(p);
  lock = open_file(&dest, L"\\writer.lock", GENERIC_WRITE, CREATE_NEW);
  if (lock == INVALID_HANDLE_VALUE) {
    status = OW_IO;
    goto done;
  }
  CloseHandle(lock);
  lock = INVALID_HANDLE_VALUE;
  lock = open_file(&dest, L"\\backup.incomplete", GENERIC_WRITE, CREATE_NEW);
  if (lock == INVALID_HANDLE_VALUE) {
    status = OW_IO;
    goto done;
  }
  if (!FlushFileBuffers(lock)) {
    status = OW_IO;
    CloseHandle(lock);
    lock = INVALID_HANDLE_VALUE;
    goto done;
  }
  CloseHandle(lock);
  lock = INVALID_HANDLE_VALUE;
  status = history_copy_directory(s, &dest, L"\\wal");
  if (status != OW_OK)
    goto done;
  status = history_copy_directory(s, &dest, L"\\checkpoints");
  if (status != OW_OK)
    goto done;
  const wchar_t *names[] = {L"\\checkpoint.current",
                            L"\\checkpoint.current.bak", L"\\recovery.current",
                            L"\\recovery.current.bak", L"\\history.policy"};
  for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    status = history_copy(s, &dest, names[i]);
    if (status == OW_NOT_FOUND)
      status = OW_OK;
    if (status != OW_OK)
      goto done;
  }
  /* Stage manifest; validation reads a provisional path only after all data is
   * flushed. */
  status = history_copy(s, &dest, L"\\manifest.ow");
  if (status != OW_OK)
    goto done;
  if (OW_FAULT(52)) {
    wchar_t *m = child(&dest, L"\\manifest.ow");
    if (m) {
      DeleteFileW(m);
      free(m);
    }
    status = OW_IO;
    goto done;
  }
  status = ow_core_database_create(&copy);
  if (status != OW_OK)
    goto done;
  status = ow_store_open_internal(destination, 0, &opened, copy, 1);
  if (status != OW_OK)
    goto done;
  status = history_same_state(db, copy, s->id);
  if (status == OW_OK) {
    ow_core_attach(copy, opened);
    status = ow_verify_database(copy, &range);
    ow_core_attach(copy, NULL);
  }
  ow_store_close(opened);
  opened = NULL;
  if (status == OW_OK) {
    wchar_t *a = child(&dest, L"\\backup.incomplete"),
            *b = child(&dest, L"\\backup.complete");
    if (!a || !b)
      status = OW_NOMEM;
    else if (OW_FAULT(53) || !MoveFileExW(a, b, MOVEFILE_WRITE_THROUGH))
      status = OW_IO;
    free(a);
    free(b);
  }
  if (status != OW_OK) {
    wchar_t *m = child(&dest, L"\\manifest.ow");
    if (m) {
      DeleteFileW(m);
      free(m);
    }
  }
done:
  ow_store_close(opened);
  ow_core_database_destroy(copy);
  if (lock != INVALID_HANDLE_VALUE)
    CloseHandle(lock);
  free(path);
  return status;
}

OwStatus ow_history_query_range(const OwHistory *view,const char *field,OwType type,
    const OwValue *lower,int lower_inclusive,const OwValue *upper,int upper_inclusive,
    OwRecordId **out,size_t *count) {
  return ow_core_query_range(view?view->db:NULL,field,type,lower,lower_inclusive,upper,upper_inclusive,out,count);
}
