# Technical debt and engineering issue register

[Maintainer handoff](HANDOFF.md) · [Development rules](DEVELOPMENT.zh-CN.md) · [Internals](IMPLEMENTATION_INTERNALS.zh-CN.md) · [Validation evidence](IMPLEMENTATION_VALIDATION.zh-CN.md)

本登记在RC文档稳定化时首次建立，按 `3f99029da73e3b4f0a75a538dd241cf94cda23d7` 的源码核验，不声称这些ID或历史取舍此前已被跟踪。性能/资源成本、已解决正确性问题和验证缺口分别列出。SQL、多写者、Linux/macOS、Last Days等产品范围之外能力不自动记债。本页不授权优化或发布。

## Record and status rules

本项目条目保留稳定TD/EG ID、描述/类别、受影响模块/函数、当前实现、源码/测试证据、成本/风险、接受或延后理由、重评触发、状态及历史；解决项保留准确commit和验证，不删除或重用ID。状态含义：Open为确认存在待决行动；Deferred为有依据延后；In Progress为获准工作正在进行；Resolved为已有解决与验证证据；Won't Fix为明确保留的决定及理由，可因新证据重开。当前没有In Progress项。

只有实际新增、改变、解决、证据/优先级变化或误分类时才更新条目，记录依据。验证缺口与确认缺陷分列；已解正确性问题保留历史缺陷分类，不混作无害成本。下面的实际债项内容、状态、证据和优先级在此次文档整理中保持不变。

## Active implementation debt

### TD-001 — insertion/removal rebuilds the global Occurrence index

- **Category / status:** performance and temporary memory / Resolved in Preview.2 development; independent acceptance pending.
- **History:** Open through accepted Preview.1. The original implementation/cost rationale below is preserved as the historical baseline, not the current implementation. Compressed nibble-radix preparation/publication removes global reconstruction; measured core and resource results and proofs are in [Preview.2](PREVIEW_2.md). LKS explicit Collection ordering remains. Reopen if a mutation again scans/rebuilds all Occurrences, sparse resources become unacceptable, or failure atomicity regresses.
- **Module:** [ordweave.c](../src/ordweave.c), `stage_occurrences`, `publish_occurrences`, occurrence insert/remove; [lks_adapter.c](../src/lks_adapter.c).
- **Current implementation:** 为插入或删除建立另一个ManagedOrder，借用全部仍存活对象（加入新项或排除待删项）。临时handles不覆盖live handles。Collection操作成功后销毁旧index并无分配地发布新handles。
- **Cost / risk:** 规模越大全局重建与临时索引内存越昂贵；一次局部成员改变可能遍历所有Occurrence。没有基准数据，不给吞吐数字或服务级别保证。
- **Why retained:** 源码体现prepare-before-publication和避免旧index内部计数回滚耗尽的正确性需要；本次没有获准性能重构，不推断更早决策记录。
- **Reconsider when:** 实测插入/删除热点和内存占用成为瓶颈，且替代方案能证明多结构失败原子性、handle和计数容量不变量。
- **Evidence / resolution:** [core.c](../tests/core.c)、[failures.c](../tests/failures.c) 对顺序、失败、容量断言；尚无resolution commit。

### TD-002 — repeated semantic replay and retained-WAL scans

- **Category / status:** recovery/history CPU and memory / Open.
- **Module:** [wal_windows.c](../src/wal_windows.c) `ow_store_open`, `replay_segment`; [history_windows.inc](../src/history_windows.inc) `history_semantic_state`, `history_reconstruct`; [checkpoint_windows.inc](../src/checkpoint_windows.inc) `cp_anchor`, `checkpoint_load`.
- **Current implementation:** structural完整扫描、直接语义重放、候选锚点扫描、native checkpoint+tail重建和逻辑比较；较早历史目标还需验证latest语义后构造target。保留多个临时core时增加峰值内存。
- **Cost / risk:** checkpoint并不使整个open只读tail；多次扫描和重放可支配启动/历史成本。统计里的tail commit数排除了semantic_scan，不能当整体工作量。
- **Why retained:** Phase 6修复防止较新checkpoint掩盖较早非法操作，见TD-101。当前可接受的是成本，不是降低语义验证强度。
- **Reconsider when:** 有可重复的大数据profile和保持等强语义审计的缓存/证明方案，并覆盖root边界、候选fallback、旧target及valid-checksummed异常。
- **Evidence / resolution:** [audit_oracle.py](../tests/audit_oracle.py)、[audit.c](../tests/audit.c)、checkpoint/history oracles；未解决，无整体启动加速承诺。

### TD-003 — whole-checkpoint temporary buffers

- **Category / status:** peak memory and bounded snapshot size / Open.
- **Module:** [checkpoint_core.inc](../src/checkpoint_core.inc) `CpWriter`, `cp_write`, `ow_core_export`, importer; [checkpoint_windows.inc](../src/checkpoint_windows.inc) `cp_load_bytes`, `cp_publish`.
- **Current implementation:** 可增长完整导出buffer，native序列化及字段排序临时数组，读取完整文件后验证/导入；格式限定checkpoint最多256MiB，文件重读和fresh core验证还有额外资源。
- **Cost / risk:** 峰值不只文件大小；可用内存或256MiB上限可使checkpoint/maintenance失败。不能将该上限误写为普通数据库所有数据总大小上限；无分配的 `ow_core_same_state` 不依赖export。
- **Why retained:** 完整摘要和图验证的直接实现，避免本次做streaming格式/恢复重构。此处“直接实现”是源码解释，不是补造历史设计决议。
- **Reconsider when:** 实际快照接近限制或峰值不可接受，且streaming方案保留精确bytes、先验长度界、故障清理和所有引用校验。
- **Evidence / resolution:** [checkpoint.c](../tests/checkpoint.c) OOM， [checkpoint_oracle.py](../tests/checkpoint_oracle.py)大小和图验证；未解决。

### TD-004 — full-copy backups

- **Category / status:** IO volume and synchronous maintenance / Deferred.
- **Module:** [history_windows.inc](../src/history_windows.inc) `history_copy`, `history_copy_directory`, `ow_backup_create`.
- **Current implementation:** 新建独立目录，64KiB缓冲逐文件复制并flush，复制保留WAL和checkpoint材料及metadata；manifest最后，private open/compare/verify后才发布complete。不是增量或去重备份。
- **Cost / risk:** 备份IO随全部保留材料增长，调用方序列化期间同步占用时间；失败可留下incomplete目标。
- **Why deferred:** 当前产品就是全量独立备份；本任务未授权增量索引、硬链接或共享生命周期设计。
- **Reconsider when:** 真实备份窗口/空间成本不可接受，并有独立可恢复、无源目录依赖及原子完成协议的需求设计。
- **Evidence / resolution:** [history.c](../tests/history.c)源改名后独立open、[audit.c](../tests/audit.c)完成fault53；未解决。增量能力是候选方向，不是当前保证。

### TD-005 — SHA provider setup for every hash

- **Category / status:** provider/handle setup overhead / Open.
- **Module:** [sha256.c](../src/sha256.c) `ow_sha256`.
- **Current implementation:** 每次 `BCryptOpenAlgorithmProvider`、`BCryptCreateHash`，两段chunked hash，Finish，DestroyHash和CloseAlgorithmProvider；所有出口归一清理。没有缓存provider。
- **Cost / risk:** 高频frame/header/metadata散列含反复OS provider初始化；未经benchmark，不量化开销。
- **Why retained:** 清晰的逐调用所有权，来自注明来源的适配；本次无授权改变线程/进程生命周期管理。
- **Reconsider when:** profile明确显示provider setup为热点，缓存方案有初始化失败、shutdown和调用方序列化之外共享状态的正确性设计。
- **Evidence / resolution:** 生产源码调用顺序；[format_oracle.py](../tests/format_oracle.py)独立hash一致性。没有provider级性能测试，未解决。

### TD-006 — interrupted unpublished files may remain

- **Category / status:** operational cleanup / Open.
- **Module:** [checkpoint_windows.inc](../src/checkpoint_windows.inc) `cp_publish`, `cp_sweep`; [history_windows.inc](../src/history_windows.inc) metadata publication and backup; [wal_windows.c](../src/wal_windows.c) pending segment recovery.
- **Current implementation:** 当前调用知道且尚未发布的temp会尝试删除；进程终止可能留下随机metadata pending、代号pending或孤立cp。可验证的旧未引用cp才由sweep删除，mandatory root代受保护；损坏/未知orphan不猜测删除。WAL next pending有专门核验/清理，并非所有pending统一忽略。失败备份目录保留incomplete标记。
- **Cost / risk:** 占用空间和运维识别负担；盲目批量删除可能破坏恢复证据或mandatory baseline。临时名不被选为权威metadata，不代表可任意删除它们。
- **Why retained:** 保守保留可疑材料优先于自动清扫未知文件；中断安全不等于自动整洁。没有获准新增维修工具。
- **Reconsider when:** 实际残留影响容量/运维，且可给出命名、身份、引用、持锁和崩溃安全证明的显式清理工具。
- **Evidence / resolution:** [checkpoint.c](../tests/checkpoint.c)发布故障、[history.c](../tests/history.c)、[audit.c](../tests/audit.c)中断边界。未解决，不承诺所有残留都会自动清理。

## Resolved correctness issues (historical, not harmless performance debt)

### TD-101 — checksummed invalid pre-checkpoint WAL bypassed semantic replay

- **Category / status:** recovery correctness / Resolved.
- **Affected module / former behavior:** `replay_segment` 在checkpoint之前检查字节/语法却跳过core操作语义，较新的有效checkpoint可掩盖无效操作。
- **Resolution:** verified commit [`15d14dc85187a1ee18b252c281511e1059e49b55`](https://github.com/RXY712200/OrdWeave/commit/15d14dc85187a1ee18b252c281511e1059e49b55) 引入独立 `history_semantic_state` 与allocation-free `ow_core_same_state`：从genesis或mandatory root baseline重放保留操作，拒绝失败/no-op/ID缺口，再比对checkpoint路径的逻辑状态和高水位。
- **Evidence:** [audit_oracle.py](../tests/audit_oracle.py) 自行重封hash和锚点的四种非法prefix；[audit.c](../tests/audit.c)各历史区间；[RELEASE_CANDIDATE](RELEASE_CANDIDATE.md)记录原回归误返回0而非CORRUPT。[修复CI](https://github.com/RXY712200/OrdWeave/actions/runs/43808017874) 与 [RC基线CI](https://github.com/RXY712200/OrdWeave/actions/runs/43809986388) MSVC两配置。
- **Remaining cost / reopen trigger:** TD-002记录成本；若更改跳过审计或root起点规则，重审该正确性约束。已物理删除的prefix不可能被重新审计，这不是此修复恢复了所有历史。

### TD-102 — unsafe backup name formatting and incomplete-marker handling

- **Category / status:** backup validation/publication correctness / Resolved.
- **Former behavior / module:** `history_copy_directory` 接受未先校验的目录名并忽略swprintf截断，190字符未知名得到NOT_FOUND而非CORRUPT；不可访问incomplete marker未正确区分IO与不存在。
- **Resolution:** 同一verified commit [`15d14dc85187a1ee18b252c281511e1059e49b55`](https://github.com/RXY712200/OrdWeave/commit/15d14dc85187a1ee18b252c281511e1059e49b55)先严格核验数字文件名/扩展名并检查格式化长度，marker hard error返回IO，完成发布失败保留incomplete。
- **Evidence:** [audit_oracle.py](../tests/audit_oracle.py)长未知名断言CORRUPT；[audit.c](../tests/audit.c)fault53后正常open拒绝目标；上述修复和RC两次CI。没有证据证明旧问题造成了任意代码执行，此处不作该推断。
- **Reopen trigger:** 放宽扫描名规则、修改目标路径长度或marker完成协议。未解决残留清理属于TD-006，不混作本修复失败。

## Evidence gaps (not established implementation defects)

### EG-001 — real hardware power-loss validation

**Status:** Deferred. Process termination和注入式IO/rename故障已有测试，但没有文件系统/存储设备/固件级突然掉电实验；FlushFileBuffers依赖外部契约。重审条件是明确的硬件耐久等级要求和实验环境。证据入口：[验证指南](IMPLEMENTATION_VALIDATION.zh-CN.md)、[持久化规范](PERSISTENCE.md)。无解决commit或“认证”结论。

### EG-002 — broader dynamic/static instrumentation

**Status:** Open. 有本库分配注入和历史GCC analyzer审查，不等于覆盖上游/OS全部分配、sanitizer全矩阵或静态分析零诊断。`cp_write` realloc诊断的现有解释保留在 [RC记录](RELEASE_CANDIDATE.md)；未将其确认为真实leak。重审条件是工具链/CI投入或新的可复现错误；工具报告须独立验证后才能升级为缺陷。无解决commit。

## Preview.1 evidence addendum

The original source-bound entries above and their status history are retained. The new isolated A/B uses exact v1.0.0 runtime with Full4.1/4.2, generated seed17 update-heavy N128/2048, three measured repeats after warmup. It measures whole operations rather than isolating TD-001 or TD-005. TD-001 remains Open: larger memory operation medians are 33.6725ms / 22.0335ms, while rebuild logic is unchanged; no attribution percentage or complete resource bound follows. Persistent operations remain approximately flat (769.4658ms / 768.2291ms). TD-002/003 remain Open: larger checkpoint/reopen/history improve in this bounded run, but no semantic pass is removed. TD-004 Deferred, TD-005/006 Open, TD-101/102 Resolved, EG-001 Deferred and EG-002 Open remain unchanged. [Preview.1 decisions](PREVIEW_1.md) ranks feasibility and explains measurement limits; raw rows and protocol are in the separate evidence branch.

### TD-103 — cleanup repaired recovery roots before rejecting busy sessions

- **Category / status:** rejected-session side effects / Resolved in the Preview.1 development change; not backported or published.
- **Module:** [history_windows.inc](../src/history_windows.inc), `ow_history_cleanup`.
- **Confirmed former behavior:** after a real retained baseline and deletion of `recovery.current.bak`, a busy session returned OW_BUSY only after recreating that file. The original source therefore performed recovery reconstruction and disk publication before its guard in `ow_verify_database`. No data loss is demonstrated.
- **Resolution:** call existing `ow_store_guard` after argument/output initialization and before recovery reconstruction; preserve success-path root repair and deletion sequencing. No new status, format or locking guarantee.
- **Evidence:** [history.c](../tests/history.c) now independently asserts missing replica stays missing for busy and COMMIT_UNKNOWN-poisoned sessions, zero output, then valid close/reopen/cleanup recovery. Before/after logs and exact eventual commit are recorded in the Preview.1 evidence handoff. Existing corruption/fault tests remain enabled.
- **Reopen trigger:** changes to cleanup entry, quarantine, root reconciliation, or pre-validation IO. Busy is not thread synchronization; callers still serialize access.

### EG-003 — intermittent checkpoint OOM return classification

**Status:** Open. First Preview.1 GCC Release full suite failed `tests/checkpoint.c` at explicit OOM boundary 126: OW_CORRUPT (8) instead of OW_NOMEM. The preserved fixture reopens successfully at Revision 21 using a native checkpoint. Targeted baseline/Full4.1, dependency-only Full4.2 and candidate runs passed; the subsequent complete Release and Debug suites each passed 12/12. No root cause, data-loss defect or upstream attribution is established. The initial log and failed database remain locally preserved; concise failure/reopen logs are archived with Preview.1 evidence. Reconsider on recurrence or independent CI failure; diagnose exact injected boundary and publication/import state before widening production scope. Rerun success does not resolve this evidence gap.

**Focused follow-up:** [EG-003 investigation](EG003_INVESTIGATION.md) inspects all24 surviving checkpoint files and records three isolated checkpoint-test runs plus five fixture-copy attempts at124–128. No CORRUPT recurrence; boundary126 reaches field payload OOM during old-checkpoint sweep and returns NOMEM unchanged. Original selection/highest generation89 suggests an earlier phase than these new generation90 sweep failures, but the removed pending bytes and pre-call state are unavailable. Exact original return site remains unknown; no speculative production fix or test relaxation was applied. Status remains Open.
