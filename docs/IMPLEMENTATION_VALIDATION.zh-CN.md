# OrdWeave v1.1.0 验证地图与可复现证据

[中文入口](../README.zh-CN.md) · [使用](USAGE.zh-CN.md) · [执行导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [内部实现](IMPLEMENTATION_INTERNALS.zh-CN.md) · [维护](DEVELOPMENT.zh-CN.md)

本文按 **OrdWeave v1.1.0** 当前源码说明；48个公开函数以 [ordweave.h](../include/ordweave.h) 为准，Full4.2 固定 `f751e1ee2afd682874281d6dc495969f9eca49eb`。实际发布状态与准确来源集中在 [HANDOFF](HANDOFF.md)，这里不把技术基线当成发布声明。 测试数量不是覆盖率或形式证明；以下明确区分源码断言、历史证据和当前执行。

## 1. 五种证据不能互相替代

| 层次 | 能支持什么 | 不能推出什么 |
|---|---|---|
|源码检查|识别实际调用顺序、所有权、分支和不变量|未执行路径一定正确、无内存安全问题|
|测试存在|仓库包含可复现断言、输入和预期|测试已运行，或断言足够强|
|固定提交的历史CI|指定提交在该机器/配置上运行成功|任意后续提交、任意设备都成功|
|本任务新运行|完成报告中命令、配置和提交实际结果|独立审查完成或全部输入正确|
|真实硬件掉电|需专门设备、文件系统、固件和断电实验|TerminateProcess、故障注入和FlushFileBuffers源码都不能代替它|

v1.0.0历史候选 `3f99029da73e3b4f0a75a538dd241cf94cda23d7` 的 [CI43809986388](https://github.com/RXY712200/OrdWeave/actions/runs/43809986388) 和Phase6修正CI是历史执行，不代表v1.1.0当前源码。已接受Preview.4实现 `5ab01df34bcb62a9feb1c61a443a1f08c0a56a06` 的 [MSVC CI45950372644](https://github.com/RXY712200/OrdWeave/actions/runs/45950372644) 两配置各12/12；其[不可变生产证据](https://github.com/RXY712200/OrdWeave/tree/41d49ed610879de00e9464f29b9cf0935a69501f/handoff/v1.1.0-preview.4/step15-implementation)含范围、预算和上游分配失败。当前精确提交的新执行结果在独立证据分支，不预填尚未产生的自SHA或CI编号。

## 2. 构建与重复执行

Windows x64，CMake 3.21+，C17编译器，Git递归submodule和Python 3。MSVC使用匹配的x64开发环境；GCC/MinGW与Ninja是另一条本地路径，不保证跨工具链二进制ABI。

```powershell
git submodule update --init --recursive
cmake -S . -B build/msvc -A x64
cmake --build build/msvc --config Debug
ctest --test-dir build/msvc -C Debug --output-on-failure
cmake --build build/msvc --config Release
ctest --test-dir build/msvc -C Release --output-on-failure
```

单配置Ninja必须使用不同目录以保留两套配置：

```powershell
cmake -S . -B build/docs-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/docs-release
ctest --test-dir build/docs-release --output-on-failure
cmake -S . -B build/docs-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/docs-debug
ctest --test-dir build/docs-debug --output-on-failure
```

[CMakeLists.txt](../CMakeLists.txt) 注册下列12个测试；[Windows workflow](../.github/workflows/windows.yml) 递归检出、核验LKS pin后，在windows-latest的Debug/Release矩阵执行同一组测试。生产库与 `OW_TESTING` 库分开，故障点和统计接口不是公开API。`ctest -N --test-dir build/docs-release` 可先列举测试。Python独立oracles由CTest传入可执行文件和fixture路径，优先通过CTest运行以保持参数一致。

## 3. 生产行为 → 真实断言地图

| CTest / 源码 | 直接验证的行为与关键断言 | 覆盖限制 |
|---|---|---|
|ordweave_smoke / [smoke.c](../tests/smoke.c)|版本串；三个调用方对象的真实LKS order、compare、move、traverse、内存serialize/destroy/restore|不创建磁盘数据库，不代替checkpoint测试|
|ordweave_core / [core.c](../tests/core.c) main、members|六类字段、UTF-8/NaN/预算、复制所有权；refs限制；四种插入/移动和no-op；ID序查询；固定种子1234567的4000步独立数组顺序模型|确定性伪随机，不是全部排列或所有分配点|
|ordweave_failures / [failures.c](../tests/failures.c)|白盒malloc/calloc失败注入，失败原子性、输出清零、adapter重复ID、ID/Revision上限、上限时合法no-op|常规CTest以本库边界为主；额外隔离上游故障证据只覆盖列出的分配序号，不是所有OS/上游组合|
|ordweave_persistence / [persistence.c](../tests/persistence.c)|第二进程BUSY；真实child退出/终止；post-write/pre-flush与post-flush/pre-report；fault 1..7；六类值；500步模型反复reopen；66次1MiB覆盖触发生产64MiB分段（最终Revision67）|没有模拟任意设备缓存损坏；reopen验证已提交状态不等于断电认证|
|ordweave_format / [format_oracle.py](../tests/format_oracle.py)|固定manifest/segment/first-frame字节，真实输出比对，44个损坏/非规范输入场景|独立编码/哈希，不是独立数据库核心|
|ordweave_checkpoint / [checkpoint.c](../tests/checkpoint.c) main、check_members|native恢复和普通尾部应用计数各1；六值/嵌NUL、重复成员、多/空Collection、高水位；keep1/2/3；分配故障和20..25/27发布边界|统计排除semantic_scan；计数1不表示整体只重放1次|
|ordweave_checkpoint_format / [checkpoint_oracle.py](../tests/checkpoint_oracle.py)|固定空checkpoint、实际非空native blob完整语法、25个损坏graph/metadata/anchor/尾部场景；区分cp真被使用和full replay fallback|不能因为fallback得到正确结果便断言native恢复正确|
|ordweave_history / [history.c](../tests/history.c) populate、crash_root、main|0/过去/latest视图与独立所有权；Manual/Forever/LastN；9段删到2段、保证35..41；每个保留Revision；source改名后备份打开；40..48/50..52故障、分配、120步模型、进程crash44|小数据cleanup可为no-op；必须检查实际文件和range而非仅OW_OK|
|ordweave_history_format / [history_oracle.py](../tests/history_oracle.py)|固定root/policy与实际root；21种损坏/缺失/不一致；段8之前前缀实际消失且35..40仍可重建|不为已经删除的历史内容提供审计|
|ordweave_audit_format / [audit_oracle.py](../tests/audit_oracle.py)|4种哈希完全有效但语义非法的pre-checkpoint操作；190字符未知checkpoint文件名应CORRUPT|有针对性的回归，不代表覆盖所有恶意语义组合|
|ordweave_adversarial / [audit.c](../tests/audit.c) audit_cases、spawn_crash|逐一检查0..14的字段、已删除ID、重复次数/顺序；cp7/9/13和尾部14；保证范围全部历史；root70..74真实TerminateProcess；备份完成53故障；结果跨view/db销毁；比较不分配且忽略字段插入顺序|进程边界与故障点固定；不模拟OS或设备突然断电|
|ordweave_external_consumer / [consumer_check.py](../tests/consumer_check.py)、[consumer/main.c](../tests/consumer/main.c)|独立临时CMake项目、BUILD_TESTING=OFF、公有target、仅ordweave.h；C17 ID/枚举断言、内存/持久化及四个索引/range函数链接调用|源码集成，不是已安装SDK或跨CRT二进制兼容测试|

## 4. 独立格式oracle为什么有价值

[format_vectors.json](../tests/format_vectors.json)、[checkpoint_vectors.json](../tests/checkpoint_vectors.json)、[history_vectors.json](../tests/history_vectors.json) 是检查入库的固定向量。Python脚本用 `struct`/整数大端转换及 `hashlib.sha256` 自己组装数据，不调用生产 `ow_encode`、checkpoint exporter或BCrypt包装来计算预期。它们同时比较真实输出并给生产reader输入异常材料，避免“同一编码器写、同一解码器读，双方共享同一个错误”成为唯一证据。

但非空LKS图的fixture仍使用真实程序获得场景，oracle独立检查其格式和关联，而不是实现第二份完整LKS。固定向量被改动时须解释每一字节变化及兼容性；不能为消除失败直接刷新golden文件。

`audit_oracle.py` 重新封装非法操作后计算合法的frame/segment链，并重封checkpoint和selection锚点。因此它不是靠旧摘要触发拒绝：缺Record删除、ID跳跃、no-op提交和缺Occurrence操作应由core重放规则拒绝。`history_semantic_state`、`replay_segment` 与 `ow_core_same_state` 是对应生产路径。见 [Resolved TD-101](TECHNICAL_DEBT.md)。

## 5. 崩溃、回收与备份的观察方法

`audit.c` 的 `spawn_crash` 创建子进程，在70..74指定点真正终止它；父进程检查子进程的预期退出值以防测试根本没到达hook。边界依次是：当前root发布前；当前已发布但bak尚旧；两root确认后删除前；首个前缀段删除后；全部删除后最终verify前。重新open验证确认提交仍在、所有保证范围可读、后来checkpoint代数不超过3，随后maintenance可继续。

`history.c`/oracle检查磁盘段数和名字，不只看内存range。备份独立性用关闭并改名隐藏源目录后再打开备份证明；[使用示例](examples/lifecycle.c)只演示两个目录独立open，不把它冒充相同强度的源不可用实验。fault53使 `backup.incomplete -> backup.complete` 发布失败，正常open必须拒绝残留incomplete目标。测试不会保证失败目标自动消失。

## 6. 分配、资源与工具的边界

核心白盒测试、checkpoint/history测试有本库分配失败计数器。重点断言失败不发布半成对象、旧值/顺序不变、创建输出零、临时core能完整析构。`ow_core_same_state` 在分配故障0时仍能比较，避免比较路径引入checkpoint总大小限制。

Phase 6记录过GCC 16.2.0 `-Wall -Wextra -Wpedantic -fanalyzer` 对 `cp_write` realloc路径的诊断；[原始说明](RELEASE_CANDIDATE.md)保留其分析，不宣称analyzer完全干净。本次文档任务不把该历史诊断视为新运行的工具结果。当前未建立全套ASan/UBSan或真实掉电认证，见 [EG-001/002](TECHNICAL_DEBT.md)。

## 7. 文档与示例的独立检查

```powershell
python tools/validate_docs.py
python tools/validate_docs.py --examples --generator Ninja --config Release
python tools/validate_docs.py --examples --generator Ninja --config Debug
```

脚本核对历史44及批准新增4个声明与48个条目、6个生产TU与5个inc/5个私有头的文档覆盖、v1.1.0文档基线、UTF-8、仓库内部Markdown文件/锚点和保护路径仅有显式授权版本差异。`--examples` 在 `build/` 临时目录单独配置 [examples/CMakeLists.txt](examples/CMakeLists.txt)，只链接公有target，运行quickstart、lifecycle及两个README共有的完整C示例；后者目录父路径由测试准备。它不修改原12个测试或生产CMake。不把字符串检查当成所有语义描述准确性的证明；仍需源码复核。

## 8. 定位准确提交的验证结果

复现和比较结果时记录准确源码SHA、命令、工具链、平台/配置、实际12项CTest与例子执行结果，以及CI run的headSha。历史日志不能当成当前commit的新执行；未跑的检查与已跑结果分开。可用以下只读命令定位当前远端和实际日志：

```powershell
git ls-remote origin refs/heads/main
gh run list --workflow windows.yml --branch main --limit 5 --json databaseId,headSha,status,conclusion,url
gh run view RUN_ID --log
```

CI成功只说明该commit在该配置执行了相应断言，不构成全部正确性证明。源码基线、已完成阶段和维护状态见 [HANDOFF](HANDOFF.md)；发布基线与兼容性证据见 [RELEASE_CANDIDATE](RELEASE_CANDIDATE.md)。

## 9. 48函数族与新增断言的真实覆盖

|公开函数族|源码断言和独立预期|边界与未证实部分|
|---|---|---|
|版本、create/destroy/open/close、release|smoke、core、persistence、external consumer；版本精确字符串、输出清零、锁与独立生命周期|不是跨CRT二进制SDK认证|
|Record/字段/revision/equality|core、failures、persistence、range_cases；六类、UTF-8、refs、预算、no-op/高水位、复制值/ID|既有字段链查找仍线性；不证明所有输入组合|
|Collection/Occurrence|4000步独立数组模型；failures radix稀疏ID/prepare abort/折叠页与容量；persistence500步重开|固定种子不等于fuzz全覆盖|
|checkpoint/retention|checkpoint与独立oracle；直接未发布import、native restore计数、分配失败、候选硬错误与anchor复用断言|native计数排除独立语义重放|
|history/policy/cleanup/backup/verify|history、audit及Python三类oracle；保证范围逐Revision、根发布/删除真实进程边界、独立备份|进程终止不等于掉电；已删历史无法审计|
|field_index_enable/disable|range_cases/index_failures；准确present/name+payload预算、失败enable保留、替换、无Revision、两个DB、teardown/exchange|固定metadata与LKS分配不计入key预算|
|query_range及匹配equality|range_cases的显式预期ID数组；Null/缺失、六类、数值极值/零、NUL/空/prefix、duplicate、全部界限flag；index_failures OOM/容量/最大ID|benchmark仅Int，不能外推所有类型性能|
|history_query_range与guard|history.c独立历史Int/当前值、跨view关闭复制结果；busy、COMMIT_UNKNOWN隔离；consumer四新函数|不是历史六类全组合独立矩阵；共享core已测六类|

维护失败测试直接检查cache/config变NULL、权威数据和Revision成功、后续不自动分配重建；no-op/失败/无关字段验证原缓存保留。上游 `LKS_ENABLE_ALLOC_DIAGNOSTICS` / `OW_UPSTREAM_ALLOC_TEST` 只在隔离测试构建启用，接受的Step15实测4个enable失败序号和2个维护失败序号均析构到上游live blocks=0，正常生产库没有hooks。不能将它冒充64个上游API重新资格测试。

## 10. 累计性能和证据预算

v1.1.0累计实验分别测A=1.0原Full4.1，B=相同1.0运行时代码仅换Full4.2，C=当前Full4.2运行时（字段索引关闭）。同机器/编译器/固定seed17 trace，在128/2048内存和128持久场景warmup+三次测量，报告配对中位数，持久setup、修改、checkpoint、reopen和history分列。C关闭/开启的字段索引另列，8192/257选择性读取及1024/19真正交错；步长17与桶数互质，并记录occupancy。旧1024/17初始值全落一个桶，只能视为低选择性历史结果，不能标均匀。

一次最终本地Debug/Release矩阵、一次文档/示例gate和最终精确HEAD MSVC矩阵足够支持本阶段执行证据。代码/测试/构建完全未变时复用既有OOM/双向兼容证据；不用反复整套运行制造信心。EG-001真实掉电、EG-002工具覆盖及EG-003 checkpoint OOM观测缺口保留，未进行独立重现campaign。新的真实失败需保留首失败状态/目录/调用点，限定追踪相关路径；不从时序噪声推导损坏。

历史包装补充：RC coverage map发现history_collection_count、occurrence_get、occurrence_compare、query_equal没有直接测试调用；history.c仅追加已知Revision40的计数6、所属ID、首末比较和counter29等值断言。原测试内容完整保留，validator核对精确插入块；只重跑受影响history，不重复本地全矩阵。
