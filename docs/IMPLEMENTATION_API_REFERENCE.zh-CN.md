# RC.1 公共 API 源码参考：44 个函数

[使用指南](USAGE.zh-CN.md) · [实现导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [内部详解](IMPLEMENTATION_INTERNALS.zh-CN.md)

依据 [include/ordweave.h](../include/ordweave.h) 在 RC.1 `3f99029da73e3b4f0a75a538dd241cf94cda23d7` 的**实际44条函数声明**整理。本文加入发布后的 main，不改变标签。每个签名直接从头文件核对；[文档校验脚本](../tools/validate_docs.py)检查清单集合及签名一致，避免凭印象猜数量。头文件仍是权威声明，不是本手册生成一个替代API。

## 1. 共同调用契约

所有输入指针必须在调用期间有效，字符串名/path须正确NUL结尾，bytes须有size字节可读；长度非零时data不可NULL。必需输出须有效、彼此独立且不与输入/内部存储别名。旧拥有结果先释放再重用。传已释放对象、失效指针、不同CRT内存或无同步并发访问不在错误状态可安全检测的范围。

持久化普通mutation/query通过会话guard：健康、busy、poisoned区别处理。参数检查与guard的先后在各入口并不完全统一，不保证多个同时非法条件的优先错误顺序。以下列出重要正常前提和失败分支，不把所有OS错误穷举成每个函数都必然返回的列表。

内存mutation在失败时不改变已发布内容/ID/Revision。持久化一旦内存应用成功后遇到提交问题，不许假装确定回滚；COMMIT_UNKNOWN隔离会话。维护IO则可能在元数据发布后的删除阶段发生。只在OW_OK时依赖成功结果；错误输出初始化不是提交结果证明。

## 2. 状态码（实际数值）

| 值 | 名称 | 调用方含义 |
|---|---|---|
|0|OW_OK|操作按其契约成功；可能是无操作|
|1|OW_INVALID|非法参数、类型/UTF-8/placement/路径、调用对象不适用|
|2|OW_NOT_FOUND|对象/字段/文件缺失；exists查询是明确例外|
|3|OW_CONFLICT|引用阻止删除、目标目录已存在、索引重复等|
|4|OW_CAPACITY|字段/Record/帧/检查点或ID/Revision/代数容量受限|
|5|OW_NOMEM|分配失败；返回部分实例不发布|
|6|OW_INTERNAL|防御性内部状态或未直接映射的LKS失败|
|7|OW_IO|Windows I/O、刷盘、hash provider等失败|
|8|OW_CORRUPT|格式、摘要、链、语义或对象图不一致|
|9|OW_UNSUPPORTED|未来/不兼容格式版本或要求字段|
|10|OW_BUSY|会话busy或OS写锁/共享冲突；不是线程锁保证|
|11|OW_COMMIT_UNKNOWN|mutation可能已经持久，输出清空；关闭重开判定|
|12|OW_RECOVERY_REQUIRED|隔离实例/必需恢复材料缺失/元数据不一致等，需要验证恢复|
|13|OW_HISTORY_UNAVAILABLE|目标晚于当前、早于基线或策略扩展要求已删历史|
|14|OW_RETENTION_UNSUPPORTED|安全维护条件不满足，例如基线槽位不足|

`io_status`把Windows sharing/lock violation映射BUSY，file/path not found映射NOT_FOUND，其余映射IO；其它调用点可能有专门分类。没有公开`ow_status_string`函数，不要从LKS的接口猜一个出来。

## 3. 类型、枚举与尺寸

- `OwDatabase`、`OwHistory` 不透明，分别独占可写core/持久会话与只读独立core；不能栈实例化、浅拷贝或相互转换。
- `OwRecordId`、`OwCollectionId`、`OwOccurrenceId` 都是 `{uint64_t value;}`，语义类型不同。零保留。计数各自递增，成功ID不复用；不能跨数据库猜来源。Database ID是单独16字节随机身份。
- `OwValue`：`type`加union，Null/Bool/Int/Float/String/Binary分别编号0..5。Bool用`int boolean`且仅0/1；Int用`int64_t integer`；Float用`double real`且有限binary64；两种bytes用`const void *data; size_t size`。字符串UTF-8可含NUL，无返回终止符保证。正负零相等；类型不自动提升。
- `OwPlacement`：FRONT=0、BACK=1、BEFORE=2、AFTER=3。端点anchor零，相对anchor活且同Collection。
- `OwRetentionPolicy`：MANUAL=0、FOREVER=1、LAST_N=2。前两种N=0，后者N>0；Last Days没有枚举/实现。
- `OwHistoryRange`：`current_revision`、`earliest_revision`为包含两端的范围，`policy`与`last_n`是策略，`int reclaimable`表示完整段前缀且槽位足够；不是字节计数或性能承诺。
- `OW_FIELD_LIMIT=1048576`、`OW_RECORD_LIMIT=8388608`。Record预算按字段`17+name字节+payload字节`求和，非RSS上限。

C17公开结构不是磁盘布局。Windows x64的源消费需要匹配枚举尺寸、packing和CRT；没有跨MSVC/GCC预编译ABI承诺。[外部consumer](../tests/consumer/main.c)对代表性的ID尺寸和status值有静态断言。

### 可用性与共同失败规则

普通Record/field/Collection/Occurrence/query/Revision操作对内存及持久化实例都适用。create/open返回新实例，identity/checkpoint/history-open/policy/range/cleanup/backup/verify要求持久化实例；历史query要求独立OwHistory而不是live OwDatabase。version与两个结果release函数不接收数据库；history_close只消耗view。下面每项补充各自的关键错误，而非重复这些规则。

普通持久化query/mutation在guard发现busy返回BUSY，poisoned返回RECOVERY_REQUIRED；mutation在core变化后的hash/写/flush不确定返回COMMIT_UNKNOWN。纯内存core没有磁盘错误；已经构造好的history query不依赖源会话健康或源文件。OOM及LKS映射错误以实际分配/容器调用路径为准。名称或bytes超过1MiB时返回CAPACITY，而非一般非法内容的INVALID；get/remove/query同样检查名称界限。测试执行证据统一见[验证指南](IMPLEMENTATION_VALIDATION.zh-CN.md)。

## 4. 函数索引

| 函数 | 用途 | 读写与Revision |
|---|---|---|
|[ordweave_version](#api-ordweave-version)|查询库版本|读取；不改变 Revision。|
|[ow_database_create](#api-ow-database-create)|新建空内存实例|初始 Revision=0；仅创建对象容器，不产生 mutation。|
|[ow_database_destroy](#api-ow-database-destroy)|兼容的实例清理入口|不追加 WAL，不产生最后一次保存。|
|[ow_database_create_persistent](#api-ow-database-create-persistent)|显式新建持久化库|新库 Revision=0；不同于静默 open-or-create。|
|[ow_database_open](#api-ow-database-open)|校验并恢复已有库|恢复已有 Revision，不递归写 WAL；普通 open 不裁剪历史。|
|[ow_database_close](#api-ow-database-close)|释放内存和 OS 会话|不提交新数据，不恢复未确认 mutation；后续必须重新 open 验证。|
|[ow_history_open](#api-ow-history-open)|获得指定 Revision 的只读视图|不改变 live Revision/ID/WAL位置；未来/已裁剪状态 HISTORY_UNAVAILABLE。|
|[ow_history_close](#api-ow-history-close)|释放历史视图|纯资源释放，无磁盘写。|
|[ow_history_revision](#api-ow-history-revision)|读取视图固定逻辑版本|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_record_exists](#api-ow-history-record-exists)|查询 Record 是否存在（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_field_get](#api-ow-history-field-get)|读取字段的独立副本（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_collection_count](#api-ow-history-collection-count)|读取成员数量并作 Collection 查找（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_collection_members](#api-ow-history-collection-members)|复制 Collection 的有序成员ID（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_occurrence_get](#api-ow-history-occurrence-get)|查询出现的不可变关联（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_occurrence_compare](#api-ow-history-occurrence-compare)|比较同Collection动态顺序（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_query_equal](#api-ow-history-query-equal)|按字段严格相等扫描Records（历史视图）|只读；返回的是视图固定Revision的状态，不随live改变。|
|[ow_history_policy](#api-ow-history-policy)|持久化保留策略|不改变数据 Revision；即使配置相同也会重新发布策略文件，不承诺免 I/O 的 no-op。|
|[ow_history_range](#api-ow-history-range)|检查保证范围及可回收性|读取，不创建历史视图句柄、不删文件，可能耗时/OOM。|
|[ow_history_cleanup](#api-ow-history-cleanup)|显式维护历史与 WAL 前缀|不增加数据 Revision；没有可回收整段时可以 OK 且无前缀删除。|
|[ow_backup_create](#api-ow-backup-create)|导出完整独立副本|不改变源数据 Revision；已有目标 CONFLICT，非法嵌套 INVALID。|
|[ow_verify_database](#api-ow-verify-database)|只读校验实际持久文件|不写磁盘、不截尾、不删文件；不代表硬件断电认证。|
|[ow_checkpoint_create](#api-ow-checkpoint-create)|发布当前已提交状态的检查点|不改变数据 Revision；同 Revision 可创建新 generation，并非 no-op。|
|[ow_checkpoint_info](#api-ow-checkpoint-info)|查看本会话实际选中的检查点|读取，不扫描文件验证所有代；验证请调用 verify。|
|[ow_checkpoint_retention](#api-ow-checkpoint-retention)|配置后续检查点代数|不改变数据 Revision，不立刻删旧代。|
|[ow_database_identity](#api-ow-database-identity)|复制持久化数据库身份|读取，无 Revision 变化。|
|[ow_revision](#api-ow-revision)|读取 live 逻辑版本|读取，不是时间戳或持久事务句柄。|
|[ow_record_create](#api-ow-record-create)|创建空 Record|成功新建 +1；已成功使用过的 ID 不复用。|
|[ow_record_delete](#api-ow-record-delete)|删除无引用 Record|成功删除 +1；重复删除缺失对象是错误而非 no-op。|
|[ow_record_exists](#api-ow-record-exists)|查询 Record 是否存在|读取，内存与持久化都支持。|
|[ow_field_set](#api-ow-field-set)|设置/替换字段副本|真正改变 +1；相等覆盖 OK/no-op，含 +/-0 相等。|
|[ow_field_get](#api-ow-field-get)|读取字段的独立副本|读取，不借出内部可变存储。|
|[ow_field_remove](#api-ow-field-remove)|移除字段|存在并删除 +1；在活Record上移除缺失字段 OK/no-op。|
|[ow_value_release](#api-ow-value-release)|释放库分配的值结果|释放结果，不修改数据库/Revision。|
|[ow_collection_create](#api-ow-collection-create)|创建空有序 Collection|成功 +1，三类ID使用独立计数器。|
|[ow_collection_delete](#api-ow-collection-delete)|删除空 Collection|成功 +1，缺失 NOT_FOUND。|
|[ow_collection_count](#api-ow-collection-count)|读取成员数量并作 Collection 查找|读取，不改变Revision。|
|[ow_collection_members](#api-ow-collection-members)|复制 Collection 的有序成员ID|读取；每个元素是OccurrenceId，不是RecordId。|
|[ow_occurrence_insert](#api-ow-occurrence-insert)|为现有对象创建独立一次出现|成功 +1；记录/集合关联在其生命周期内不改变。|
|[ow_occurrence_move](#api-ow-occurrence-move)|移动同集合内的指定出现|实际改变 +1；self、已有正确端点/邻接 OK/no-op。|
|[ow_occurrence_remove](#api-ow-occurrence-remove)|删除一次出现|成功 +1；不删除它引用的Record。|
|[ow_occurrence_get](#api-ow-occurrence-get)|查询出现的不可变关联|读取，允许同Record多次出现。|
|[ow_occurrence_compare](#api-ow-occurrence-compare)|比较同Collection动态顺序|读取；不能用ID或指针数值替代此比较。|
|[ow_query_equal](#api-ow-query-equal)|按字段严格相等扫描Records|读取；不是二级索引、SQL或类型自动转换。|
|[ow_array_release](#api-ow-array-release)|释放库返回数组|不修改数据库/Revision。|

## 5. 逐函数约定

<a id="api-ordweave-version"></a>

### ordweave_version

```c
const char *ordweave_version(void);
```

- **用途与参数**：查询库版本。无参数，当前字符串为 1.0.0-rc.1；不读取磁盘。
- **输出、所有权**：返回静态库拥有的 const char*，不释放。
- **可观察变化/无操作**：读取；不改变 Revision。
- **关键失败**：无OwStatus失败通道；无需database或初始化。
- **实现路径**：[src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/smoke.c](../tests/smoke.c)。

<a id="api-ow-database-create"></a>

### ow_database_create

```c
OwStatus ow_database_create(OwDatabase **out);
```

- **用途与参数**：新建空内存实例。out 必需。calloc 后创建三个 ID 索引；任一失败清理全部已建部分。无磁盘 I/O。
- **输出、所有权**：先置 *out=NULL，成功交出独占实例；用 close/destroy 释放。
- **可观察变化/无操作**：初始 Revision=0；仅创建对象容器，不产生 mutation。
- **关键失败**：NULL out为INVALID；calloc或index创建失败为NOMEM；只在全部容器成功后返回OK。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-database-destroy"></a>

### ow_database_destroy

```c
void ow_database_destroy(OwDatabase *db);
```

- **用途与参数**：兼容的实例清理入口。内部调用 close 并丢弃状态；NULL 可用。不能据此判断之前的提交成功。
- **输出、所有权**：正常/隔离实例被消耗；BUSY close 不消耗，destroy 也不会强拆忙碌对象。
- **可观察变化/无操作**：不追加 WAL，不产生最后一次保存。
- **关键失败**：void无状态回传；忙碌实例不会被强拆，因此调用方不能通过destroy隐藏串行化错误。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-database-create-persistent"></a>

### ow_database_create_persistent

```c
OwStatus ow_database_create_persistent(const char *path, OwDatabase **out);
```

- **用途与参数**：显式新建持久化库。path 见 Windows 路径契约，必须不存在且父目录存在；先建目录和锁，初段刷盘后发布 manifest。已存在返回 CONFLICT。
- **输出、所有权**：out 先 NULL；失败不交出部分实例，可能留下新建的不完整目录。
- **可观察变化/无操作**：新库 Revision=0；不同于静默 open-or-create。
- **关键失败**：非法路径/NULL out为INVALID，已有目录CONFLICT，锁共享冲突BUSY，分配NOMEM，Windows/hash失败IO；失败可能留下不完整目录。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/wal_windows.c](../src/wal_windows.c)。
- **可定位的验证**：[tests/persistence.c](../tests/persistence.c)。

<a id="api-ow-database-open"></a>

### ow_database_open

```c
OwStatus ow_database_open(const char *path, OwDatabase **out);
```

- **用途与参数**：校验并恢复已有库。独占 writable 会话；检查 manifest/root/WAL、语义重放、原生检查点和结果一致性。拒绝损坏/未来格式，可能协调最终不完整尾。
- **输出、所有权**：out 先 NULL；完整校验后才交出实例。失败不替换为空库。
- **可观察变化/无操作**：恢复已有 Revision，不递归写 WAL；普通 open 不裁剪历史。
- **关键失败**：非法参数INVALID，缺路径/文件按路径分类NOT_FOUND或恢复所需；锁冲突BUSY，坏字节/语义CORRUPT，未来格式UNSUPPORTED，缺mandatory材料RECOVERY_REQUIRED，IO/NOMEM不静默fallback。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/wal_windows.c](../src/wal_windows.c)。
- **可定位的验证**：[tests/persistence.c](../tests/persistence.c)、[tests/audit_oracle.py](../tests/audit_oracle.py)。

<a id="api-ow-database-close"></a>

### ow_database_close

```c
OwStatus ow_database_close(OwDatabase *db);
```

- **用途与参数**：释放内存和 OS 会话。NULL 返回 OK。BUSY 时返回且不释放。隔离实例返回 RECOVERY_REQUIRED 但仍释放。调用后自己将变量置 NULL。
- **输出、所有权**：除 BUSY 外消耗 db，不可再次访问/关闭原指针。已取出的结果副本仍有效。
- **可观察变化/无操作**：不提交新数据，不恢复未确认 mutation；后续必须重新 open 验证。
- **关键失败**：NULL/健康内存或持久实例OK；BUSY不消耗；poisoned返回RECOVERY_REQUIRED但消耗。没有“close再提交”分支。
- **实现路径**：[src/persistence.c](../src/persistence.c)。
- **可定位的验证**：[tests/persistence.c](../tests/persistence.c)。

<a id="api-ow-history-open"></a>

### ow_history_open

```c
OwStatus ow_history_open(OwDatabase *db, uint64_t revision, OwHistory **out);
```

- **用途与参数**：获得指定 Revision 的只读视图。仅健康持久化实例。目标必须在实际保证范围内；完整保留链也要有效。先重建，最后才分配 OwHistory。
- **输出、所有权**：out 先 NULL；视图拥有独立 core，用 history_close。源实例可后续修改/关闭。
- **可观察变化/无操作**：不改变 live Revision/ID/WAL位置；未来/已裁剪状态 HISTORY_UNAVAILABLE。
- **关键失败**：内存库/NULL out为INVALID；目标范围外HISTORY_UNAVAILABLE；完整链/图错误CORRUPT，需协调短尾RECOVERY_REQUIRED，资源错误IO/NOMEM，版本UNSUPPORTED。
- **实现路径**：[src/history_windows.inc: history_reconstruct_impl](../src/history_windows.inc)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-close"></a>

### ow_history_close

```c
void ow_history_close(OwHistory *view);
```

- **用途与参数**：释放历史视图。NULL 无操作。关闭释放其独立数据库；不接收 writable OwDatabase。
- **输出、所有权**：消耗 view；已经取得的值/数组副本独立。不要重复关闭同一非 NULL 指针。
- **可观察变化/无操作**：纯资源释放，无磁盘写。
- **关键失败**：void；NULL无操作，非NULL必须是尚未释放的拥有view。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc)。
- **可定位的验证**：[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-revision"></a>

### ow_history_revision

```c
OwStatus ow_history_revision(const OwHistory *view, uint64_t *out);
```

- **用途与参数**：读取视图固定逻辑版本。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：*out 先0；仅 OK 时为有效视图Revision。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/out为INVALID；无分配、无源文件读取。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-record-exists"></a>

### ow_history_record_exists

```c
OwStatus ow_history_record_exists(const OwHistory *view, OwRecordId id, int *out);
```

- **用途与参数**：查询 Record 是否存在（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：*out 先0，成功0或1，无新分配结果。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/out或零ID为INVALID；非零缺失ID仍OK/0。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-field-get"></a>

### ow_history_field_get

```c
OwStatus ow_history_field_get(const OwHistory *view, OwRecordId id, const char *name, OwValue *out);
```

- **用途与参数**：读取字段的独立副本（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：out 先置 Null/零；成功结果用 value_release；无额外NUL保证。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/out、非法name或零ID为INVALID；缺Record/字段NOT_FOUND；bytes复制可能NOMEM。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-collection-count"></a>

### ow_history_collection_count

```c
OwStatus ow_history_collection_count(const OwHistory *view, OwCollectionId id, size_t *out);
```

- **用途与参数**：读取成员数量并作 Collection 查找（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：*out先0；不分配。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/out或零ID为INVALID，缺Collection为NOT_FOUND；空Collection为OK/0。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-collection-members"></a>

### ow_history_collection_members

```c
OwStatus ow_history_collection_members(const OwHistory *view, OwCollectionId id, OwOccurrenceId **out, size_t *count);
```

- **用途与参数**：复制 Collection 的有序成员ID（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：out先NULL、count先0；空为NULL/0；非空数组用array_release。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/out/count或零ID为INVALID，缺Collection为NOT_FOUND，数组尺寸CAPACITY，分配NOMEM。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-occurrence-get"></a>

### ow_history_occurrence_get

```c
OwStatus ow_history_occurrence_get(const OwHistory *view, OwOccurrenceId id, OwCollectionId *collection, OwRecordId *record);
```

- **用途与参数**：查询出现的不可变关联（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：两种ID输出先0；成功为所属Collection/Record，无LKS句柄泄漏。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/必需输出或零ID为INVALID，缺Occurrence为NOT_FOUND。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-occurrence-compare"></a>

### ow_history_occurrence_compare

```c
OwStatus ow_history_occurrence_compare(const OwHistory *view, OwOccurrenceId a, OwOccurrenceId b, int *out);
```

- **用途与参数**：比较同Collection动态顺序（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：*out先0；成功负/零/正；同一出现比较0。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/out、零ID或跨Collection为INVALID，缺Occurrence为NOT_FOUND；活同一ID为OK/0。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-query-equal"></a>

### ow_history_query_equal

```c
OwStatus ow_history_query_equal(const OwHistory *view, const char *name, const OwValue *value, OwRecordId **out, size_t *count);
```

- **用途与参数**：按字段严格相等扫描Records（历史视图）。view必需且仍存活；其余参数、缺失/非法规则与对应live查询相同。直接查询view拥有的core，不重新读取源文件。
- **输出、所有权**：out先NULL/count先0，空结果OK；数组按RecordId递增，用array_release。
- **可观察变化/无操作**：只读；返回的是视图固定Revision的状态，不随live改变。
- **关键失败**：NULL view/输出、非法name/value为INVALID；结果数组尺寸CAPACITY或分配NOMEM；缺字段不返回NOT_FOUND而是不匹配。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-history-policy"></a>

### ow_history_policy

```c
OwStatus ow_history_policy(OwDatabase *db, OwRetentionPolicy policy, uint64_t last_n);
```

- **用途与参数**：持久化保留策略。Manual/Forever 要 last_n=0；LastN 要 >0。不能扩大到已丢失范围。设置不删文件；可能 I/O 或发布不确定。
- **输出、所有权**：无结果对象；发布成功才更新会话配置。未知发布进入隔离。
- **可观察变化/无操作**：不改变数据 Revision；即使配置相同也会重新发布策略文件，不承诺免 I/O 的 no-op。
- **关键失败**：内存库或policy/N组合非法INVALID，LastN请求超出已删范围HISTORY_UNAVAILABLE；发布可能IO/NOMEM，rename后结果不确定RECOVERY_REQUIRED。
- **实现路径**：[src/history_windows.inc: history_publish](../src/history_windows.inc)。
- **可定位的验证**：[tests/history.c](../tests/history.c)。

<a id="api-ow-history-range"></a>

### ow_history_range

```c
OwStatus ow_history_range(OwDatabase *db, OwHistoryRange *out);
```

- **用途与参数**：检查保证范围及可回收性。仅持久化实例；重建最早状态，不只是读结构。LastN 再扫描段边界。reclaimable 要存在完整旧段且至少两个代槽。
- **输出、所有权**：out 入口清零；只在 OK 时使用全部字段。后续边界检查错误时不要依赖部分标量内容。
- **可观察变化/无操作**：读取，不创建历史视图句柄、不删文件，可能耗时/OOM。
- **关键失败**：内存库/NULL out为INVALID；重建最早态传播CORRUPT、UNSUPPORTED、RECOVERY_REQUIRED、IO/NOMEM；LastN边界扫描失败时输出可能已有部分标量。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc)。
- **可定位的验证**：[tests/history.c](../tests/history.c)。

<a id="api-ow-history-cleanup"></a>

### ow_history_cleanup

```c
OwStatus ow_history_cleanup(OwDatabase *db, OwHistoryRange *out);
```

- **用途与参数**：显式维护历史与 WAL 前缀。验证真实状态，必要时先协调已选恢复根，构造新基线/后缀，发布两根并验证后才删除。Manual/Forever 不推进范围，但可完成旧清理残留。
- **输出、所有权**：out 入口清零，成功返回验证范围。失败不一定等于维护回滚；隔离须重开。
- **可观察变化/无操作**：不增加数据 Revision；没有可回收整段时可以 OK 且无前缀删除。
- **关键失败**：不适用参数INVALID；不足保留代槽RETENTION_UNSUPPORTED，代号/epoch溢出CAPACITY；验证错误传播，发布不确定RECOVERY_REQUIRED；IO可能在根已发布后发生。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-backup-create"></a>

### ow_backup_create

```c
OwStatus ow_backup_create(OwDatabase *db, const char *destination);
```

- **用途与参数**：导出完整独立副本。仅健康持久化源；目标新绝对路径，不可位于源目录内部（含规范化父路径检查）。验证源后复制、刷盘、独立恢复比较，再完成标记。
- **输出、所有权**：destination 是借用字符串。失败可能留下新目录；不覆盖旧目标，不返回新会话。
- **可观察变化/无操作**：不改变源数据 Revision；已有目标 CONFLICT，非法嵌套 INVALID。
- **关键失败**：不适用参数/非法或源内嵌套路径INVALID；已有目标CONFLICT；源验证错误传播，创建/复制/flush/完成rename可能IO/NOMEM；失败不把目标标为完成。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc)。
- **可定位的验证**：[tests/history.c](../tests/history.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-verify-database"></a>

### ow_verify_database

```c
OwStatus ow_verify_database(OwDatabase *db, OwHistoryRange *out);
```

- **用途与参数**：只读校验实际持久文件。检查根对、重建当前状态、检查选择对和引用检查点，再确认最早范围。比正常 open 的回退策略更严格；不自动修复。
- **输出、所有权**：out 入口清零，仅 OK 时使用；损坏、不支持、缺冗余/材料、IO/NOMEM 区分。
- **可观察变化/无操作**：不写磁盘、不截尾、不删文件；不代表硬件断电认证。
- **关键失败**：不适用参数INVALID；root缺文件可NOT_FOUND，root/selection副本不一致RECOVERY_REQUIRED；损坏CORRUPT，未来格式UNSUPPORTED，IO/NOMEM明确返回；不修复短尾。
- **实现路径**：[src/history_windows.inc](../src/history_windows.inc)。
- **可定位的验证**：[tests/history_oracle.py](../tests/history_oracle.py)、[tests/audit_oracle.py](../tests/audit_oracle.py)。

<a id="api-ow-checkpoint-create"></a>

### ow_checkpoint_create

```c
OwStatus ow_checkpoint_create(OwDatabase *db);
```

- **用途与参数**：发布当前已提交状态的检查点。仅持久化健康会话。导出/写/刷盘/重读导入验证/发布/同步选择备份/清理旧代。已裁剪实例且代槽<2 返回 RETENTION_UNSUPPORTED。
- **输出、所有权**：无新句柄。IO 可能发生在新检查点已经发布后的清理阶段；info/reopen 可确认。
- **可观察变化/无操作**：不改变数据 Revision；同 Revision 可创建新 generation，并非 no-op。
- **关键失败**：内存库INVALID，busy/poisoned受guard；基线槽不足RETENTION_UNSUPPORTED，导出或generation容量CAPACITY，分配NOMEM；IO或发布后不确定RECOVERY_REQUIRED须区分。
- **实现路径**：[src/checkpoint_windows.inc](../src/checkpoint_windows.inc)。
- **可定位的验证**：[tests/checkpoint.c](../tests/checkpoint.c)。

<a id="api-ow-checkpoint-info"></a>

### ow_checkpoint_info

```c
OwStatus ow_checkpoint_info(const OwDatabase *db, uint64_t *generation, uint64_t *revision);
```

- **用途与参数**：查看本会话实际选中的检查点。仅持久化会话，两个必需且不同的输出地址。返回的是实际选择/恢复结果，可能是回退到较旧代或恢复根基线。
- **输出、所有权**：两个输出先 0；无检查点时 OK/0/0。
- **可观察变化/无操作**：读取，不扫描文件验证所有代；验证请调用 verify。
- **关键失败**：内存库或任一必需输出NULL为INVALID；健康时不读文件，不因为磁盘上存在坏孤儿就由此报告CORRUPT。
- **实现路径**：[src/checkpoint_windows.inc](../src/checkpoint_windows.inc)。
- **可定位的验证**：[tests/checkpoint.c](../tests/checkpoint.c)。

<a id="api-ow-checkpoint-retention"></a>

### ow_checkpoint_retention

```c
OwStatus ow_checkpoint_retention(OwDatabase *db, unsigned generations);
```

- **用途与参数**：配置后续检查点代数。generations 必须 1..3，默认2；只修改本会话并随下一份检查点选择元数据落盘。与 Last N 历史策略不是一件事。
- **输出、所有权**：无拥有结果；锚定库设置1可成功，但后续需要基线槽的维护可能拒绝。
- **可观察变化/无操作**：不改变数据 Revision，不立刻删旧代。
- **关键失败**：内存库或generations不在1..3为INVALID；健康setter不做IO，不证明下一次发布会成功。
- **实现路径**：[src/checkpoint_windows.inc](../src/checkpoint_windows.inc)。
- **可定位的验证**：[tests/checkpoint.c](../tests/checkpoint.c)。

<a id="api-ow-database-identity"></a>

### ow_database_identity

```c
OwStatus ow_database_identity(const OwDatabase *db, unsigned char out[16]);
```

- **用途与参数**：复制持久化数据库身份。out 指向至少16字节，实例须持久化且健康。此身份在 reopen/backup 保留，不是对象ID。
- **输出、所有权**：输出先清16字节；纯内存/NULL/隔离状态错误。成功字节属于调用方。
- **可观察变化/无操作**：读取，无 Revision 变化。
- **关键失败**：NULL out或非持久实例INVALID；会话guard可BUSY/RECOVERY_REQUIRED；本函数不重新读取manifest。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/wal_windows.c](../src/wal_windows.c)。
- **可定位的验证**：[tests/persistence.c](../tests/persistence.c)。

<a id="api-ow-revision"></a>

### ow_revision

```c
OwStatus ow_revision(const OwDatabase *db, uint64_t *out);
```

- **用途与参数**：读取 live 逻辑版本。db 与 out 必需；持久化实例先检查隔离/busy。新库为0。
- **输出、所有权**：*out 先0；仅 OK 时为有效当前Revision。
- **可观察变化/无操作**：读取，不是时间戳或持久事务句柄。
- **关键失败**：NULL db/out为INVALID；持久化guard可BUSY/RECOVERY_REQUIRED；无分配。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-record-create"></a>

### ow_record_create

```c
OwStatus ow_record_create(OwDatabase *db, OwRecordId *out);
```

- **用途与参数**：创建空 Record。预检查 counter/Revision 容量，分配稳定 Record，再唯一插入 ID索引，最后提交计数。
- **输出、所有权**：out.value 先0，成功为新非零ID；持久化未知提交时仍是0，不说明未提交。
- **可观察变化/无操作**：成功新建 +1；已成功使用过的 ID 不复用。
- **关键失败**：NULL db/out为INVALID；counter或Revision满CAPACITY；节点/index分配NOMEM；持久化未知提交仍不交出ID。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)、[tests/failures.c](../tests/failures.c)。

<a id="api-ow-record-delete"></a>

### ow_record_delete

```c
OwStatus ow_record_delete(OwDatabase *db, OwRecordId id);
```

- **用途与参数**：删除无引用 Record。非零ID；缺失 NOT_FOUND，refs>0 CONFLICT。索引先去除借用，再释放字段/Record。
- **输出、所有权**：无结果对象；失败不隐式删除 Occurrences。
- **可观察变化/无操作**：成功删除 +1；重复删除缺失对象是错误而非 no-op。
- **关键失败**：NULL db或零ID为INVALID，缺Record为NOT_FOUND，仍有引用CONFLICT，真正删除时Revision满CAPACITY。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-record-exists"></a>

### ow_record_exists

```c
OwStatus ow_record_exists(OwDatabase *db, OwRecordId id, int *out);
```

- **用途与参数**：查询 Record 是否存在。ID非零；缺失非零ID返回OK/false，不同于 get/delete。
- **输出、所有权**：*out 先0，成功0或1，无新分配结果。
- **可观察变化/无操作**：读取，内存与持久化都支持。
- **关键失败**：NULL db/out或零ID为INVALID；非零缺失ID是OK/0，无分配。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-field-set"></a>

### ow_field_set

```c
OwStatus ow_field_set(OwDatabase *db, OwRecordId id, const char *name,
                      const OwValue *value);
```

- **用途与参数**：设置/替换字段副本。Record须存在；名称和值有效。先判断严格类型相等，再检查预算与Revision容量，准备拷贝后交换；持久化包装先预编码。
- **输出、所有权**：输入只借用到返回；覆盖释放数据库旧副本，调用者的旧返回副本仍有效。
- **可观察变化/无操作**：真正改变 +1；相等覆盖 OK/no-op，含 +/-0 相等。
- **关键失败**：非法db/id/name/value为INVALID；缺Record为NOT_FOUND；payload/name/Record预算或Revision容量CAPACITY，准备复制NOMEM；no-op可绕过core容量但持久预编码仍可能失败。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)、[tests/persistence.c](../tests/persistence.c)。

<a id="api-ow-field-get"></a>

### ow_field_get

```c
OwStatus ow_field_get(OwDatabase *db, OwRecordId id, const char *name,
                      OwValue *out);
```

- **用途与参数**：读取字段的独立副本。Record与字段必须存在；缺失 NOT_FOUND。String/Binary再分配复制，可能NOMEM。
- **输出、所有权**：out 先置 Null/零；成功结果用 value_release；无额外NUL保证。
- **可观察变化/无操作**：读取，不借出内部可变存储。
- **关键失败**：非法db/id/name/out为INVALID；缺Record或字段NOT_FOUND；bytes复制NOMEM。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-field-remove"></a>

### ow_field_remove

```c
OwStatus ow_field_remove(OwDatabase *db, OwRecordId id, const char *name);
```

- **用途与参数**：移除字段。Record须存在且name有效；找到字段后断链、扣预算、释放。
- **输出、所有权**：无拥有结果；删除不影响此前取得的副本。
- **可观察变化/无操作**：存在并删除 +1；在活Record上移除缺失字段 OK/no-op。
- **关键失败**：非法db/id/name为INVALID；缺Record为NOT_FOUND；活Record缺字段OK；真正删除时Revision满CAPACITY。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-value-release"></a>

### ow_value_release

```c
void ow_value_release(OwValue *value);
```

- **用途与参数**：释放库分配的值结果。只对返回的拥有值使用，非借用输入；NULL可用。String/Binary释放缓冲。
- **输出、所有权**：结构重置全零（OW_NULL）；不要对拥有同一指针的复制结构重复释放。
- **可观察变化/无操作**：释放结果，不修改数据库/Revision。
- **关键失败**：void；NULL无操作；非拥有结果、重复释放或失效指针不属于可用状态码检测的输入。
- **实现路径**：[src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)、[tests/audit.c](../tests/audit.c)。

<a id="api-ow-collection-create"></a>

### ow_collection_create

```c
OwStatus ow_collection_create(OwDatabase *db, OwCollectionId *out);
```

- **用途与参数**：创建空有序 Collection。准备 Collection 和空 LksOrder，再唯一插入 Collection索引。失败销毁准备对象。
- **输出、所有权**：out.value先0，成功为新ID；空Collection有效。
- **可观察变化/无操作**：成功 +1，三类ID使用独立计数器。
- **关键失败**：NULL db/out为INVALID；ID/Revision容量CAPACITY，对象/Order/index分配NOMEM或adapter映射的LKS错误。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-collection-delete"></a>

### ow_collection_delete

```c
OwStatus ow_collection_delete(OwDatabase *db, OwCollectionId id);
```

- **用途与参数**：删除空 Collection。非零活ID；非空 CONFLICT，不级联清成员；去索引后销毁空Order与对象。
- **输出、所有权**：无结果；成员须由调用方先逐项remove。
- **可观察变化/无操作**：成功 +1，缺失 NOT_FOUND。
- **关键失败**：非法db/零ID为INVALID，缺Collection为NOT_FOUND，非空CONFLICT，真正删除时Revision满CAPACITY。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-collection-count"></a>

### ow_collection_count

```c
OwStatus ow_collection_count(OwDatabase *db, OwCollectionId id, size_t *out);
```

- **用途与参数**：读取成员数量并作 Collection 查找。非零活Collection；缺失NOT_FOUND，与空Collection数量0区分。
- **输出、所有权**：*out先0；不分配。
- **可观察变化/无操作**：读取，不改变Revision。
- **关键失败**：非法db/id/out为INVALID，缺Collection为NOT_FOUND；不分配结果。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-collection-members"></a>

### ow_collection_members

```c
OwStatus ow_collection_members(OwDatabase *db, OwCollectionId id,
                               OwOccurrenceId **out, size_t *count);
```

- **用途与参数**：复制 Collection 的有序成员ID。两个必需独立输出；按原生Order遍历，不按ID排序；检查数组乘法界。
- **输出、所有权**：out先NULL、count先0；空为NULL/0；非空数组用array_release。
- **可观察变化/无操作**：读取；每个元素是OccurrenceId，不是RecordId。
- **关键失败**：非法db/id/out/count为INVALID，缺Collection为NOT_FOUND，数组尺寸CAPACITY，分配NOMEM。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)、[tests/history.c](../tests/history.c)。

<a id="api-ow-occurrence-insert"></a>

### ow_occurrence_insert

```c
OwStatus ow_occurrence_insert(OwDatabase *db, OwCollectionId collection,
                              OwRecordId record, OwPlacement placement,
                              OwOccurrenceId anchor, OwOccurrenceId *out);
```

- **用途与参数**：为现有对象创建独立一次出现。Collection/Record均活；placement/anchor符合规则。准备新Occurrence与完整替代索引，成功插入Order后无分配发布。
- **输出、所有权**：out.value先0；每次重复成员都返回独立ID，无去重。
- **可观察变化/无操作**：成功 +1；记录/集合关联在其生命周期内不改变。
- **关键失败**：非法参数/placement/anchor关系INVALID；缺Record/Collection/相对anchor为NOT_FOUND；ID/Revision/refs容量CAPACITY；暂存索引或Order失败不发布对象。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-occurrence-move"></a>

### ow_occurrence_move

```c
OwStatus ow_occurrence_move(OwDatabase *db, OwOccurrenceId id,
                            OwPlacement placement, OwOccurrenceId anchor);
```

- **用途与参数**：移动同集合内的指定出现。id活；front/back anchor=0；before/after anchor同Collection。交给LKS公开move。
- **输出、所有权**：无新ID/数组；移动保留该出现的身份。
- **可观察变化/无操作**：实际改变 +1；self、已有正确端点/邻接 OK/no-op。
- **关键失败**：非法id/placement/anchor所属关系INVALID，缺出现或相对anchor NOT_FOUND；真正移动时Revision/LKS容量可CAPACITY；self或已有正确位置OK/no-op。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-occurrence-remove"></a>

### ow_occurrence_remove

```c
OwStatus ow_occurrence_remove(OwDatabase *db, OwOccurrenceId id);
```

- **用途与参数**：删除一次出现。先准备排除该项的替代索引，再去除Order residence，发布索引、减Record.refs、释放Occurrence。
- **输出、所有权**：无结果；删除后原ID缺失，其它出现保持有效。
- **可观察变化/无操作**：成功 +1；不删除它引用的Record。
- **关键失败**：非法db/零ID为INVALID，缺出现NOT_FOUND；真正删除时Revision满CAPACITY，替代全局索引分配可能NOMEM。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-occurrence-get"></a>

### ow_occurrence_get

```c
OwStatus ow_occurrence_get(OwDatabase *db, OwOccurrenceId id,
                           OwCollectionId *collection, OwRecordId *record);
```

- **用途与参数**：查询出现的不可变关联。活非零Occurrence；collection与record两个不同的必需输出。
- **输出、所有权**：两种ID输出先0；成功为所属Collection/Record，无LKS句柄泄漏。
- **可观察变化/无操作**：读取，允许同Record多次出现。
- **关键失败**：非法db/零ID/必需输出NULL为INVALID，缺出现NOT_FOUND；不分配、不返回内部对象地址。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-occurrence-compare"></a>

### ow_occurrence_compare

```c
OwStatus ow_occurrence_compare(OwDatabase *db, OwOccurrenceId left,
                               OwOccurrenceId right, int *out);
```

- **用途与参数**：比较同Collection动态顺序。两个出现都活且同Collection；缺失NOT_FOUND，跨集合INVALID。
- **输出、所有权**：*out先0；成功负/零/正；同一出现比较0。
- **可观察变化/无操作**：读取；不能用ID或指针数值替代此比较。
- **关键失败**：非法db/id/out或跨Collection为INVALID，缺出现NOT_FOUND；LKS比较错误经adapter映射。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-query-equal"></a>

### ow_query_equal

```c
OwStatus ow_query_equal(OwDatabase *db, const char *name, const OwValue *value,
                        OwRecordId **out, size_t *count);
```

- **用途与参数**：按字段严格相等扫描Records。名称和值有效；两次按Record ID索引扫描，先计数后分配填充。缺失字段不匹配Null。
- **输出、所有权**：out先NULL/count先0，空结果OK；数组按RecordId递增，用array_release。
- **可观察变化/无操作**：读取；不是二级索引、SQL或类型自动转换。
- **关键失败**：非法db/name/value/输出为INVALID，数组容量CAPACITY或分配NOMEM；没有匹配是OK/NULL/0，不是NOT_FOUND。
- **实现路径**：[src/persistence.c](../src/persistence.c) → [src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

<a id="api-ow-array-release"></a>

### ow_array_release

```c
void ow_array_release(void *array);
```

- **用途与参数**：释放库返回数组。接受NULL；只传本库拥有数组，不传栈/输入/已经释放的地址。
- **输出、所有权**：函数不清调用方变量；释放后自行置NULL。
- **可观察变化/无操作**：不修改数据库/Revision。
- **关键失败**：void；NULL可用；函数不清空调用方指针，重复释放不是合法输入。
- **实现路径**：[src/ordweave.c](../src/ordweave.c)。
- **可定位的验证**：[tests/core.c](../tests/core.c)。

## 6. 生命周期与常见误用

| 取得资源 | 拥有者 | 释放 | 失效点 |
|---|---|---|---|
| database create/open | 调用者 | close/destroy | 正常/隔离close消耗；BUSY例外 |
| history open | 调用者 | history_close | 关闭视图；源关闭不影响它 |
| field get | 调用者独立副本 | value_release | 自己释放，不是下一次mutation |
| members/query数组 | 调用者独立副本 | array_release | 自己释放 |
| version字符串 | 库 | 不释放 | 静态存储 |
| LKS Handle | 从不公开 | 由内部容器管理 | 不允许调用者拿它作为ID |

不要释放借用的输入bytes；不要复制拥有结果后双重释放；不要因close返回RECOVERY_REQUIRED而再次close同一指针；不要在UNKNOWN后自动重试create；不要把checkpoint_retention当Last N；不要把历史视图强转为OwDatabase；不要因当前无SQL就把原生顺序等同于按字段排序。

使用层完整实例见[使用指南](USAGE.zh-CN.md)。追踪分配和发布点见[实现导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md)及[内部详解](IMPLEMENTATION_INTERNALS.zh-CN.md)。

<a id="preview4-step15"></a>

## Preview.4 Step15：已实现的会话索引与范围查询

当前未发布版本包含历史44个函数和本节4个新增函数，共48个；RC.1/Stable 的44函数历史基线不变。头文件是声明权威。

每个数据库最多一个显式启用的字段索引，覆盖该字段所有六类现存值；缺失字段不入索引，Null 入索引。max_records 是有该字段的条目数上限，不是数据库总记录数。max_key_bytes 包含一份字段名（含末尾 NUL）及 String/Binary 键复制的有效字节数；固定行、ID、值元数据和 LKS 分配不计入此预算，因此它不是总内存上限。两个限制必须为正；超预算返回 OW_CAPACITY，分配失败返回 OW_NOMEM。enable 私下构建，成功才替换；失败保留旧索引。disable 幂等释放。配置不占 Revision/永久 ID，不写 WAL，重开后不存在。

成功的数据修改后动态维护相关键；维护分配、插入、删除或预算失败会释放**索引和配置**，不改变已成功修改的结果。随后查询扫描；只有再次显式 enable 才重建。失败、无操作和无关字段修改保留已证实有效的索引。关闭、销毁和 core exchange 在旧对象释放前清除缓存；历史重建、导入和回放不自动建立缓存。busy/隔离会话沿用原 guard，COMMIT_UNKNOWN 后关闭并验证重开，不能盲目重试。

范围查询只选择指定类型，不转换类型。两个界限可各自为 NULL（无界，不是 Null 值）；非空界限须匹配 type 并通过原值校验，Float 必须有限。两个 inclusive 即使对应界限缺省也必须为0或1。下界大于上界返回 OW_INVALID；相等且任一侧排除返回空成功。排序为值升序、相同值按永久 Record ID 升序：Null 单值、Bool false 在前、Int 有符号数值、Float 数值（正负零相等）、String/Binary 无符号字节词典序后比较长度，支持嵌入 NUL 和空跨度，不使用语言区域排序。缺失字段排除。

有匹配索引时查询使用公共 ManagedOrder；否则范围查询扫描后使用公共 LKS Stable Sort。原等值查询可利用匹配索引，结果仍按 ID 升序。历史范围查询在视图自己的不可变 core 中扫描排序，不共享 live 缓存。所有结果为独立 ID 数组，使用 ow_array_release；空/错误为 NULL/0，分配失败不返回部分成功。调用方仍须串行化访问。查询结果可在修改、禁用、关闭源或关闭历史视图后使用，直到调用方释放。

<a id="api-ow-field-index-enable"></a>

### ow_field_index_enable

```c
OwStatus ow_field_index_enable(OwDatabase *db, const char *field,
                              size_t max_records, size_t max_key_bytes);
```

遵循本节共同契约；live 函数先通过会话 guard，history 函数使用独立视图。

<a id="api-ow-field-index-disable"></a>

### ow_field_index_disable

```c
OwStatus ow_field_index_disable(OwDatabase *db);
```

遵循本节共同契约；live 函数先通过会话 guard，history 函数使用独立视图。

<a id="api-ow-query-range"></a>

### ow_query_range

```c
OwStatus ow_query_range(OwDatabase *db, const char *field, OwType type,
                       const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
```

遵循本节共同契约；live 函数先通过会话 guard，history 函数使用独立视图。

<a id="api-ow-history-query-range"></a>

### ow_history_query_range

```c
OwStatus ow_history_query_range(const OwHistory *view, const char *field,
                       OwType type, const OwValue *lower, int lower_inclusive,
                       const OwValue *upper, int upper_inclusive,
                       OwRecordId **out, size_t *count);
```

遵循本节共同契约；live 函数先通过会话 guard，history 函数使用独立视图。
