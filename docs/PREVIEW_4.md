# Preview.4 — accepted research and approved Step15 implementation

Unreleased `1.1.0-preview.4`. Research Steps13–14 are accepted; the approved Step15 continuation implements exactly four additive functions (48 total). Independent implementation acceptance remains pending. No RC, merge or release is authorized. The following research sections preserve their original research-time scope; the implemented contract below supersedes proposal-only wording.

## Decision matrix

| Capability / real use | Appropriate public LKS calls | Evidence / cost | Decision |
|---|---|---|---|
|Repeated named-field equality / filtered sorted UI|ManagedOrder create/insert/locate/remove/next, immutable copied keys with permanent-ID tie|Actual DB scans vs derived index;128/8192 records, selective/nonselective, reads and update/delete maintenance, memory and failure fallback|Worthwhile opt-in query/index contract proposed below; retain existing automatic scan implementation until explicit approval.|
|One-shot sorted query result|`lks_sort`, `lks_group_build`|Independent stable insertion-sort model;8192 pointer results about0.2483/0.2541ms|Use supported LKS sort if an ordered-query API is approved. No replacement of canonical qsort or forced production use now.|
|Recurring immutable sorted batches|GroupBatch build/merge_all; Group build/merge|8192 build+merge0.2562ms; two reusable Groups64 merges1.8057ms; borrowed lifetime explicitly tested|Useful implementation option for future read results; no separate batch API or transaction proposal.|
|Collection changes across revisions|Permanent Occurrence IDs over independent OwHistory results; public Snapshot/Cursor/LS1 only for domain/lifetime checks|Actual persistent views verify inserted6/deleted2/reordered5 and changed Record1; insertion-only rank shift is not movement; pruned view rejected|Do not add a public diff API now. Small quadratic LCS is research only; ambiguous movement and scalable result contract need more justification. Existing copied arrays permit client-side analysis.|
|Canonical checkpoint field sort|Existing unique-name qsort|No duplicate name stability need, unchanged canonical bytes/failure semantics|Retain; no allocating replacement.|

## Step13: semantics and lifecycle

Actual live `ow_query_equal` guards the session, then scans Record ManagedOrder twice and returns increasing permanent Record IDs with one exact result allocation. History delegates to the same core query on an independently reconstructed immutable core. Fields own name/value bytes. Set/remove/delete, WAL apply, native import, core exchange and destruction all affect any derived-cache lifetime; a hidden automatic maintained index would require coherent handling across all of them. Existing no-op, failed mutation, quarantine and result ownership remain unchanged in production.

Prototype keys are immutable copies while resident. Comparator order is presence, type, value, permanent Record ID: missing is distinct from present Null; mixed types never coerce; signed Int comparison avoids subtraction overflow; finite Float uses numeric order with both zeros equal; String/Binary compares unsigned byte spans then lengths, including embedded NUL. Public input validation must reject nonfinite Float before comparison. The measured named field is integer-valued; comparator unit assertions cover missing/Null, type separation, zeros and NUL/prefix byte cases, not a complete production index implementation for every type.

ManagedOrder does not provide transactions/reserve/bulk construction. The private prototype performs the successful DB mutation, then removes the old immutable key before replacing its row key and inserting. Any maintenance removal/capacity or insertion/OOM failure destroys the derived container and falls back to the existing scan. It never attempts allocating inverse rollback of the database, exposes stale keys or turns cache-maintenance failure into a failed committed mutation. All Row payloads remain alive until the borrowed index is destroyed. A linker-injected actual allocation failure during maintenance verifies fallback results against the model. This is a bounded external prototype, not a complete native cache lifecycle integrated into replay/import/exchange.

### Paired protocol and results

Protocol frozen before measuring: exact Preview.3 runtime, seed17-derived deterministic field distributions,128/8192 populations,257-value selective and1-value nonselective cases,512 reads, one warmup and three measured runs. Mixed workload is a separate phase of50 updates/50 deletions followed by512 reads, not interleaved traffic. Setup/build/maintenance/read spans are separate. Initial update-only and borrowed-row measurements are retained as exploratory evidence; the final index separately owns its copied integer-key Row arena, and both allocation and materialization are included in build cost; the mixed update/delete matrix is the reported final set. Baseline/indexed order alternates. Read spans conservatively include independent expected-result scans in both variants; they are not isolated lookup timings.

|8192 workload median total build+maintenance+reads|Scan ms|Indexed prototype ms|
|---|---:|---:|
|Selective, reads only|51.8508|2.8532|
|Selective, mixed maintenance|47.9283|2.8232|
|Nonselective, reads only|49.9864|25.2955|
|Nonselective, mixed maintenance|50.3880|25.2967|

Estimated amortization from component medians is roughly a few dozen reads in the measured selective cases (exact component estimates are in the final report). These estimates are not directly measured single-query break-even guarantees and include the common oracle cost. One-shot queries pay a build cost and copied-key memory; hidden automatic maintenance is therefore not selected solely on repeated-read wins. Resource runs separately measure requested allocation, not RSS:8192 selective read-only built requested bytes about1.50MB scan/common model versus2.19MB indexed, roughly0.69MB extra including a separately allocated copied-key Row arena in this integer fixture. Strings/Binary would require copied payload accounting beyond this measured workload.

### One ranked public addition — explicit approval required

Rank1: **bounded opt-in named-field ordered/range querying**, with an optional live derived index. No other public addition is ranked. Suggested function family below is a concrete review proposal, not declarations added to the public header:

```c
OwStatus ow_field_index_enable(OwDatabase *db, const char *field,
                              size_t max_records, size_t max_key_bytes);
OwStatus ow_field_index_disable(OwDatabase *db);
OwStatus ow_query_range(OwDatabase *db, const char *field, OwType type,
                       const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
OwStatus ow_history_query_range(const OwHistory *view, const char *field,
                       OwType type, const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
```

Proposed precise behavior:

- Existing types/statuses/result release only; no persisted metadata. One active named-field index per live DB, process/session-local and explicitly enabled. Positive limits cap indexed entries and owned copied-key bytes, not exact upstream/OS resident bytes. Enable prepares a full replacement before publishing; INVALID/NOMEM/CAPACITY preserves prior enable state. Disable is idempotent. Configuration consumes no logical Revision or permanent ID and is not WAL-logged.
- Range query works whether an index is enabled/valid or not. Missing fields excluded; present Null is a real typed value. Bounds are optional pointers (NULL means unbounded), non-NULL bounds must have the explicit requested type; finite Float, canonical zeros and existing bytes/name validation apply. Inclusivity is0/1; lower>upper returns INVALID; equal bounds with either exclusive returns an empty result. No mixed-type coercion. Result is ascending typed value, then increasing permanent Record ID; this intentionally differs from existing equality's ID-only order and requires approval. Ascending-only first version avoids an unnecessary direction contract.
- Output array is an independently owned copy released by `ow_array_release`, initialized NULL/zero on error; empty succeeds with NULL/zero. Current guard/error validation precedes work; invalid argument→INVALID, resource failure→NOMEM or CAPACITY, busy/quarantine→existing statuses. It does not expose runtime Handles, keys, borrowed values or a public cursor. Query itself does not advance Revision.
- Cache is advisory: failure/budget exhaustion during best-effort maintenance invalidates it without changing a successful data mutation's status, IDs or Revision. Subsequent queries scan/sort or explicitly rebuild under the same serialized operation; they never trust Revision alone after replay/import/exchange. Failed/no-op mutations can retain a known-valid index; invalidation is always safe. Commit-unknown/recovery-required blocks access through existing guards. Mutation/replay/import/exchange/destruction must drop or rebuild owned derived state before releasing old keys/objects. No cache survives close/open or transfers silently between independent DBs.
- Immutable history query uses the view's own reconstructed state and copied temporary results; no live index is shared and no historical audit is bypassed. A view unavailable through pruning must be obtained through existing `ow_history_open` and remains unavailable. No new view-source/cursor lifetime guarantee is implied.
- First implementation may scan and use public LKS Stable Sort, then add the explicit ManagedOrder optimization with bounded failure/lifecycle tests. Equivalent cached/uncached results and fallback are mandatory. Approximate prototype break-even is not an API latency promise.

At research HEAD cf342dcf this family was unapproved and unimplemented. The owner subsequently approved this exact family within Step15; the implemented contract below governs the current extension. This does not authorize RC.1.

## Step14: stable sorted results and immutable groups

The sort prototype compares copied pointer-array LKS Stable Sort, immutable Group build, GroupBatch build/merge_all and two Groups built then merged. An independent stable insertion-sort reference checks every resulting pointer/ID with equal-key runs. Input is ID-ascending, so stable equal-key order also gives permanent-ID ties; general batch merge must instead use an explicit ID tie comparator or preserve known input chunk precedence. Base wins comparator equality on Group merge; arbitrary independent streams do not automatically produce global ID ties.

At8192 rows, median sort0.2482ms, Group0.2541ms, Batch+merge0.2562ms, two-build+merge0.2513ms. Reusing both immutable groups for64 owned merges takes1.8057ms. Build/copy costs are included; no claim that Batch is best for one-shot work. Groups borrow rows, batch owns its groups, merge creates an owned container, and copied DB results must own IDs/values independently. Destroy groups before source Row storage. No alternate sorting/index library, generic importer, durable batch mutation or commit reordering is added.

## Step15: historical/order analysis and rejected extra API

Actual persistent historical views outlive the live database; copied arrays remain valid after view closure. Example before order[1,2,3,4,5], after[5,1,3,4,6] yields inserted6/deleted2 and retained longest common subsequence[1,3,4], hence a canonical reordered candidate5. Shared Record1's integer field changes17→31. Insertion-only[1,2,3]→[9,1,2,3] preserves all relative order despite every old rank shifting.

The prototype's deterministic LCS tie rule skips the earlier sequence on equal alternatives. It is quadratic, bounded to tiny inputs and not proposed as a production large-diff algorithm. Views alone cannot uniquely recover explicit move operations from WAL; a reordered candidate is a minimal relative-order explanation, not an action log. Duplicate Record references are distinguished by permanent Occurrence IDs. A production diff would need source identity embedded with views, a scalable retained-ID/LIS mapping and explicit ambiguous-order/result ownership semantics; existing public history does not expose a source-identity getter. No public diff contract is selected without that justification.

Public LKS cursor invalidation is asserted after an actual move, before accessing cached handles. Cursors are destroyed before their owner. Snapshot association bytes are copied and key text remains valid after live Order destruction; same-domain keys compare, different revision namespaces reject comparison. LS1 coordinates are snapshot-local, never permanent IDs or cross-domain diff keys. A separate real retained-root fixture rejects earliest-1 with HISTORY_UNAVAILABLE; no fabricated old state is returned.

## Validation, debt and scope

All prototype files and raw/resource/failure/history results are on `evidence/v1.1.0-preview.4` under `handoff/v1.1.0-preview.4/`; no unused prototype enters production. Only runtime version and matching smoke/consumer literals change, so no new persistence compatibility campaign is needed. Existing suites/docs and exact-head Windows CI are reported at handoff. Historical RC/Stable and accepted Preview.1–3 validator gates remain; current production bytes must exactly equal accepted Preview.3 apart from version literal. No API or data-path gate is relaxed.

Technical Debt Impact: no existing TD status changes. TD-001's index improvement, TD-002/003 partial mitigations, TD-004 deferred, TD-005/006 open and TD-103 resolved history remain. EG-001/002/003 remain; no EG-003 standalone reproduction campaign. The proposed features are pending product decisions, not automatically technical debt. Research completion is distinct from public capability implementation and independent acceptance.


## Preview.4 Step15：已实现的会话索引与范围查询

当前未发布版本包含历史44个函数和本节4个新增函数，共48个；RC.1/Stable 的44函数历史基线不变。头文件是声明权威。

每个数据库最多一个显式启用的字段索引，覆盖该字段所有六类现存值；缺失字段不入索引，Null 入索引。max_records 是有该字段的条目数上限，不是数据库总记录数。max_key_bytes 包含一份字段名（含末尾 NUL）及 String/Binary 键复制的有效字节数；固定行、ID、值元数据和 LKS 分配不计入此预算，因此它不是总内存上限。两个限制必须为正；超预算返回 OW_CAPACITY，分配失败返回 OW_NOMEM。enable 私下构建，成功才替换；失败保留旧索引。disable 幂等释放。配置不占 Revision/永久 ID，不写 WAL，重开后不存在。

成功的数据修改后动态维护相关键；维护分配、插入、删除或预算失败会释放**索引和配置**，不改变已成功修改的结果。随后查询扫描；只有再次显式 enable 才重建。失败、无操作和无关字段修改保留已证实有效的索引。关闭、销毁和 core exchange 在旧对象释放前清除缓存；历史重建、导入和回放不自动建立缓存。busy/隔离会话沿用原 guard，COMMIT_UNKNOWN 后关闭并验证重开，不能盲目重试。

范围查询只选择指定类型，不转换类型。两个界限可各自为 NULL（无界，不是 Null 值）；非空界限须匹配 type 并通过原值校验，Float 必须有限。两个 inclusive 即使对应界限缺省也必须为0或1。下界大于上界返回 OW_INVALID；相等且任一侧排除返回空成功。排序为值升序、相同值按永久 Record ID 升序：Null 单值、Bool false 在前、Int 有符号数值、Float 数值（正负零相等）、String/Binary 无符号字节词典序后比较长度，支持嵌入 NUL 和空跨度，不使用语言区域排序。缺失字段排除。

有匹配索引时查询使用公共 ManagedOrder；否则范围查询扫描后使用公共 LKS Stable Sort。原等值查询可利用匹配索引，结果仍按 ID 升序。历史范围查询在视图自己的不可变 core 中扫描排序，不共享 live 缓存。所有结果为独立 ID 数组，使用 ow_array_release；空/错误为 NULL/0，分配失败不返回部分成功。调用方仍须串行化访问。查询结果可在修改、禁用、关闭源或关闭历史视图后使用，直到调用方释放。

实现位于 [field_index.inc](../src/field_index.inc)，由 ordweave.c 拥有；仍为六个生产翻译单元，现有五个实现 include。四个声明及所有旧类型/状态由文档校验器与精确研究 HEAD 比较。Step15 实现和验证完成不代表独立验收；后续 RC 工作尚未授权。

## Step15 production verification summary

Windows x64 MinGW/GCC Release, exact research HEAD cf342dcf scan versus actual candidate APIs; one warmup plus three measured repetitions, same deterministic model, 512 reads each. Independent expected-result scans and result release are outside query timing. Totals include enable/build and successful update/delete API time, excluding initial fixture setup. Range benchmark uses equal inclusive bounds so its result has the same model as equality; broader range contracts are independently tested, not performance-characterized by this matrix.

|Fixture|Old equality scan ms|Candidate equality disabled ms|Equality enabled ms|Range disabled ms|Range enabled ms|
|---|---:|---:|---:|---:|---:|
|8192 records / 257 values / 0 changes phased|46.9603|47.1584|1.6973|73.4728|1.8123|
|8192 records / 257 values / 100 changes phased|48.0178|48.9073|1.7796|73.7159|1.8263|
|1024 records / 17 values / 64 changes interleaved|4.5935|4.3278|3.3089|26.5601|3.2529|
|128 records / 1 values / 0 changes phased|0.5205|0.5340|0.4307|2.6160|0.4293|

Selective read-only equality is about 27.67x faster including construction. Component-median estimated build amortization is 15.4 reads versus disabled equality; this is an estimate, not a measured single-query guarantee. Disabled equality total deltas in this bounded matrix are +0.42%, +1.85%, -5.78%, +2.59%; none meets the predeclared material threshold (>10% and >1ms). Small/nonselective construction costs may outweigh benefits for one-shot use; no general speedup guarantee.

Separate requested-byte accounting (not RSS): 8192 integer records, disabled candidate setup 1,365,813 bytes versus old 1,300,268 (+65,545, principally an 8-byte private Record backlink and one DB pointer). Allocation counts are identical (33,037); disabled mode constructs no index. Enabling adds 758,108 requested bytes and 16,559 allocations, including owned rows and public ManagedOrder. Integer keys consume only the copied field name's 4-byte variable-key budget; fixed metadata/LKS memory lies outside that budget. Post-destroy wrapper residual is 117 bytes in both enabled and disabled candidate, versus 116 baseline; no additional index residual. These are wrapper-observed process allocations, not a whole-process leak or resident-memory certification.

Local full CTest Debug and Release:12/12 each. Isolated upstream failures:4 enable and2 maintenance allocation ordinals; existing2 insert ordinals also pass with zero upstream live blocks. Stable and accepted Preview.3 bilateral checkpoint/history/backup round trips pass, including enabled candidate source, and diagnostic public query allocation counts prove cache absence on reopen. Canonical checkpoint fixture:386 bytes, SHA-256 `655f001cf7f3b790c2df7775f2cd04c19be0bce67eb15a48ab246ef42f836958` in Stable, Preview.3, candidate disabled/enabled. No persistent encoding or dependency change. Exact-head MSVC CI and immutable raw evidence are recorded in the Step15 completion report on the existing evidence branch. EG-003 remains Open; this bounded suite did not reproduce its checkpoint OOM boundary and is not a repeated OOM campaign.
