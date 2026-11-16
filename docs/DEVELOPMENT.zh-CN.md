# OrdWeave 开发与维护规则

[交接入口](HANDOFF.md) · [内部实现](IMPLEMENTATION_INTERNALS.zh-CN.md) · [验证地图](IMPLEMENTATION_VALIDATION.zh-CN.md) · [技术债](TECHNICAL_DEBT.md)

本指南提供公开的技术维护方法；仓库边界和验证入口见 [AGENTS](../AGENTS.md)。本指南描述 `3f99029da73e3b4f0a75a538dd241cf94cda23d7`（RC.1）的组织及今后的维护约束。产品行为、兼容性和恢复契约不能因文档维护而改变；发布状态与实际证据见 [RELEASE_CANDIDATE](RELEASE_CANDIDATE.md)。

## 1. 先定位正确层次

| 目录/文件 | 用途 | 修改入口 |
|---|---|---|
|include/ordweave.h|唯一公开C契约|所有签名、status数值、结构布局变更先做兼容评审|
|src/ordweave.c + checkpoint_core.inc|纯core、对象所有权和图导入导出|不把Windows IO或WAL递归写入core|
|src/persistence.c|公开wrapper与mutate|入口guard、输出安全、准备/执行/持久确认顺序|
|src/lks_adapter.c/.h|LKS公共API边界|集中status翻译、ID唯一性、handle规则|
|src/wal_format.c + wal_internal.h|确定性命令编码、decode、apply|写/读canonical规则必须一致；oracle独立|
|src/wal_windows.c + checkpoint_windows.inc + history_windows.inc|文件、锁、扫描、metadata、root、history、backup|依赖同TU的static helper；inc不是独立对象文件|
|src/sha256.c|BCrypt SHA包装|明确provider/hash生命周期|
|tests/ 与 tests/consumer/|原12项CTest与独立调用方|production库无启用故障hooks|
|docs/examples/、tools/validate_docs.py|隔离的文档示例和检查|不改变产品构建或既有测试语义|
|third_party/LayerKeySort/|固定独立上游submodule|只用其公有API，不在此仓库修改上游|

[CMakeLists.txt](../CMakeLists.txt)构建C17 static `ordweave`，公开别名 `OrdWeave::ordweave`；LKS与bcrypt是实现依赖。依赖自己的测试、示例、benchmarks、capacity screen和install被关闭。根project不提供安装包。调用方可 `set(BUILD_TESTING OFF CACHE BOOL "" FORCE)` 后add_subdirectory并链接别名，见 [USAGE](USAGE.zh-CN.md)。完整构建/CTest命令见 [验证指南](IMPLEMENTATION_VALIDATION.zh-CN.md)，不要给同一构建目录切换generator或CRT。

## 2. 从公有调用找到实现

以 `ow_field_set` 为例：先读公开声明，再读persistence.c wrapper和mutate，随后查 `core_names.h` 的 `ow_core_field_set` 重命名关系，在ordweave.c找函数；持久化另外跟 `ow_encode/ow_apply`、`ow_store_commit`、`replay_segment`。只搜索一个同名函数会漏掉包装后的真实顺序。history facade在history_windows.inc转发到独立core，checkpoint导入则在core inc访问对象布局。完整时序见 [导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md)。

```powershell
rg -n 'ow_field_set|mutate|ow_core_field_set' include src
rg -n 'ow_encode|ow_store_commit|replay_segment|history_semantic_state' src
```

查bug时先保留失败目录、命令、准确SHA和状态；检查实际落盘值与内存值是否属于同一Revision。`OW_COMMIT_UNKNOWN` 不能通过盲目重试“修好”；关闭并重开确认后再决定后续操作。

## 3. 改变操作的评审顺序

1. 写清用户可见的新行为、no-op、错误优先级、输出初始值、内存/持久化适用范围及Revision影响。先判断是否改变现有公共契约或磁盘语义。
2. 确认core可单独执行，成功改变状态恰增1，失败/no-op不消耗ID；容量边界下no-op仍不应被误报容量失败。持久化预编码可能先于core对象查找，不能随意重排错误优先级。
3. 列出分配/借用/发布点：名字/bytes深复制，Record refs按Occurrence计算，结果由匹配release释放。先完成所有可能失败的准备，再进行不能回滚的可见状态转移；无法保证时要设计明确的隔离状态而不是假回滚。
4. 逐个检查command、encode/decode/apply、commit补ID、replay预期ID/Revision、checkpoint图与历史重建；新增内存行为不能漏记WAL。
5. 检查immutable IDs、native LKS namespace/association和高水位；增加真实行为测试、独立格式/语义异常输入和失败路径断言。
6. 运行适配范围的测试，再跑原12项及Windows两配置。记录实际失败/限制，独立评审后才能称接受。
7. 更新API、使用、执行链、格式规范、验证地图和确实受影响的债项。不要机械修改所有文档或每次都改技术债登记。债项有实质变化时保留对应ID、源码和验证依据；无变化不机械改登记。

## 4. 兼容性是显式边界

WAL/manifest/segment/frame保持v3，checkpoint/selection/root/policy各v1，LKS native `LKS4SNP1`。精确布局仅由 [PERSISTENCE](PERSISTENCE.md)、[CHECKPOINTS](CHECKPOINTS.md)、[HISTORY](HISTORY.md)集中维护。不能在保留版本号时改变长度、offset、reserved=0规则、digest范围、负零规范化、name/value限制或祖先链含义。新版本/迁移需要单独授权设计和旧fixture兼容证据。

持久日志中每帧必须是一个真正改变状态且Revision连续的合法操作。即使checksum正确，也必须通过core语义；较新checkpoint不能隐藏较早无效操作。root明确声明被删前缀边界，不意味着reader可自行把缺失segment当新genesis。root mandatory baseline不可随普通checkpoint fallback被丢弃。修改metadata顺序必须重新列崩溃状态，并证明current/bak删除前共同确认。

不要把SHA误写成身份认证。备份只能新建，incomplete标记先于复制；独立验证后最后完成发布。不能为了便利在open中自动压缩、删除历史或修复不应修复的完整损坏。

## 5. LKS与资源不变量

ID比较只比较不可变uint64值，不用可能溢出的相减。ManagedOrder locate的pred/equal/succ必须是三个独立输出；插入前保证ID唯一。容器借用的对象先活于容器，析构先detach/destroy容器再free对象。一个Occurrence只能属于一个Collection，其永久ID、所属Record与Collection不可在移动时改写。移动只改变本Collection顺序。

runtime Handle不持久化，不把指针或Handle伪装成数据库ID。restore必须通过Occurrence ID resolver校验所属Collection、唯一出现和namespace，并重建每个新Handle。任何container replacement都必须更新对象保存的index_handle。全局Occurrence index重建目前有成本但服务于失败原子性，优化前须保留失败/handle证据，见TD-001。

所有文件、锁、BCrypt句柄、临时core、数组和buffer应有单一所有者及所有出口的清理策略。输出先安全初始化，失败不暴露半成对象；IO确认前不发布成功ID；不确定提交要poison会话。跨函数返回缓冲区须说明谁free、何时失效。不要对借用输入调用ow_value_release。

## 6. 测试与fixtures的维护

普通core场景用公开API检查独立预期数组/值，而不是再调用实现helper计算预期。原子性测试在改变前保存状态，注入后比较值、顺序、refs、Revision与下次ID；仅检查“返回了NOMEM”不够。历史测试逐一选取边界前、边界、checkpoint之间、tail和当前Revision；使用明确的预期集合，避免把当前态错误地当每个历史态。

格式oracle需独立编码并重算所有相关摘要，确保异常针对语义而不是意外坏hash。golden修改必须解释格式与兼容；真实crash父进程须确认hook到达。备份独立测试应使源不可用；cleanup断言实际文件删除、保证范围仍可读且小于边界返回HISTORY_UNAVAILABLE。复用现有独立consumer模式，不让它递归启用全部测试。

本次文档例子的验证是隔离CMake项目，不向原12测试追加产品功能。新测试的名称和断言发生变化时，同步 [验证地图](IMPLEMENTATION_VALIDATION.zh-CN.md)。

## 7. 可维护性原则与债务登记

模块按责任分工，避免没有收益的耦合与权威逻辑复制。名字说明对象及动作；把有清晰前后条件的步骤拆成有意义函数，而不是为行数机械拆分。避免无必要的长函数和深层嵌套，但历史代码重构须有单独范围与回归证据。注释解释非显然的意图、借用寿命、不变量及崩溃顺序，不重复C语句。格式文档避免另起竞争规范。

[TECHNICAL_DEBT.md](TECHNICAL_DEBT.md) 的ID永久稳定。引入、显著改变、偿还或明确接受真实债务时更新对应项，附源码、理由、触发条件及验证；解决后移入历史区但保留ID和提交SHA。功能范围之外的SQL、多写者、其他平台不自动成为欠账；测试证据缺口单独EG编号。未经证实的缺陷标为待复核，不能把正确性bug包装成可接受性能成本。

## 8. 发布基线与维护边界

当前已发布版本为 `v1.0.0-rc.1` Prerelease，源码基线为 `3f99029da73e3b4f0a75a538dd241cf94cda23d7`；Stable尚未发布。保留现有tag和Release，新的文档提交不进入历史标签。当前项目的格式、依赖、调用方串行化和资源所有权约束继续适用，具体证据见 [RELEASE_CANDIDATE](RELEASE_CANDIDATE.md)。

原六个实现阶段已完成，见 [ROADMAP](ROADMAP.md)。普通维护不会自动开启新产品阶段、改上游或创建版本。维护时核对实际remote与 [HANDOFF](HANDOFF.md) 中的固定基线，使用本指南的源码/格式/测试地图，不将旧文档的任务上下文当作当前修改权限。
