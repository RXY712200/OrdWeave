# OrdWeave v1.1.0 使用指南

[中文入口](../README.zh-CN.md) · [48 个 API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [实现导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md)

本文按 **OrdWeave v1.1.0** 当前源码说明；48个公开函数以 [ordweave.h](../include/ordweave.h) 为准，Full4.2 固定 `f751e1ee2afd682874281d6dc495969f9eca49eb`。实际发布状态与准确来源集中在 [HANDOFF](HANDOFF.md)，这里不把技术基线当成发布声明。

## 1. 先理解要保存什么

Record 是字段的所有者；Collection 保存有序的成员；Occurrence 表示“某 Record 在某 Collection 中的一次出现”。同一 Record 可出现四次而只保存一份字段，每次出现都有不同的 Occurrence ID，可以分别移动和删除。更改 Record 的字段会被所有引用它的出现共享；移动某次出现不会修改 Record 的数据。

适合表达有序清单、图层成员、编排条目等嵌入式数据。它没有 SQL、联网服务、自动/持久化二级字段索引、多写者并发或跨 Collection 原子转移。字段相等默认扫描；可显式启用单字段缓存加速重复查询，范围结果支持值升序，仍不是通用查询优化器。

## 2. 取得源码、构建与外部链接

支持 Windows x64、C17、CMake 3.21+。需要 Git；启用测试需要 Python 3。MSVC 和 GCC/MinGW 的 Debug/Release 有执行证据。不要把“代码使用 C17”解释为 Linux/macOS 已获支持；当前根 CMake 明确拒绝非 Windows 后端。

```powershell
# 在已选定的 v1.1.0 源码检出目录执行（准确来源见 HANDOFF）
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

GCC/Ninja 配置改用 `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`；不要给 Ninja 传 `-A x64`。已有 clone 用 `git submodule update --init --recursive` 补齐依赖。GitHub 自动 ZIP/tar 不嵌入子模块，不是自包含源码包。标签包含产品代码；要阅读这套新文档，另看当前 main，不要移动标签。

外部工程可以采用以下完整 CMake 项目，`ORDWEAVE_SOURCE_DIR` 指向完整 clone：

```cmake
cmake_minimum_required(VERSION 3.21)
project(MyApp LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(BUILD_TESTING OFF CACHE BOOL "Embed OrdWeave" FORCE)
add_subdirectory("${ORDWEAVE_SOURCE_DIR}" ordweave)
add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE OrdWeave::ordweave)
```

`main.c` 可直接使用[完整内存示例](examples/quickstart.c)。公开目标传递头文件与静态链接需求；不要手工拼接私有 LKS 头或仅拷贝一个 `.a` 后忽略依赖。使用相同编译器、CRT、枚举尺寸和结构打包设置；不承诺 MSVC/GCC 预编译二进制互换。当前不提供安装包或共享 DLL 安装规则。

## 3. 最小内存程序

[quickstart.c](examples/quickstart.c) 是可编译运行的完整程序，创建一条 Record，设置整数，创建 Collection，再将该 Record 分别插到首尾。返回的两个 Occurrence ID 必须不同，成员数组必须以最后一次 front 插入为首。

它先创建 Record，再创建引用它的 Occurrence，是为了满足引用完整性。成员数组和 `ow_field_get` 的结果是独立副本；在 `done` 路径各释放一次，然后销毁数据库。销毁数据库会释放其所有对象，不需要先逐个删除。这种 `ow_database_create` 实例完全没有磁盘路径，不能用于检查点、持久化历史或备份。

失败就沿清理路径退出，不继续使用未成功取得的 ID。三个 ID 类型各包装 `uint64_t value`，零无效；它们不是 LKS 指针，也不携带跨数据库来源验证。不能把数据库 A 的 ID 当作数据库 B 的同名对象。

## 4. 持久化实例与 Windows 路径

[lifecycle.c](examples/lifecycle.c) 是完整持久化程序，执行所有字段类型、四种插入和移动、检查点、历史查询、保留策略、显式清理、备份、关闭及分别重开。调用时提供两个**尚不存在**的绝对路径，父目录必须已经存在：

```powershell
.\lifecycle.exe D:/data/new-ordweave D:/backups/new-ordweave-copy
```

程序不会覆盖旧目录，不自动重试失败操作，也不会递归删除数据。`ow_database_create_persistent` 用于新建，`ow_database_open` 用于已有库；打开失败不是新建许可。存在但不完整、损坏或不支持的库必须先判断错误，不能删掉后假装是空库。

路径输入是 NUL 结尾 UTF-8，内部严格转成 UTF-16。允许绝对盘符路径和 `/`、`\` 分隔符；拒绝相对路径、UNC、设备/扩展命名空间、`.`/`..`、ADS、保留 DOS 设备名、`~` 别名、末尾点或空格、重复/末尾空组件、reparse 祖先。完整输入最多 239 个 UTF-16 code units，不含终止零。父目录不存在时也不能新建中间目录。使用本地普通目录；敌对命名空间竞态不是这里的安全保证。

同一个库持有全会话 Windows 文件锁。第二个写会话通常返回 `OW_BUSY`，不因为路径上残留 `writer.lock` 就永远判忙。不要手工删除活跃实例的文件来“解锁”；异常进程退出后 OS 会释放句柄。所有线程访问仍需调用方串行化，包括读与销毁。

`ow_database_close` 消耗正常或隔离中的实例；返回 `OW_BUSY` 时不消耗。`ow_database_destroy` 是忽略 close 状态的兼容清理入口，不能用它判断业务操作是否持久化。关闭不生成一次最后保存；每个已确认改变状态的调用此前已经写 WAL 并刷盘。

## 5. 值、名字与所有权

`OwValue.type` 决定读取哪个 union 成员，不能给一个类型填另一个成员。

| 类型 | 输入成员 | 规则 |
|---|---|---|
| `OW_NULL` | 无 | 与“字段不存在”不同 |
| `OW_BOOL` | `as.boolean` | 只接受 0/1 |
| `OW_INT` | `as.integer` | 有符号 64 位 |
| `OW_FLOAT` | `as.real` | 有限 binary64，拒绝 NaN/Inf；正负零相等 |
| `OW_STRING` | `as.bytes.data/size` | 有效 UTF-8；长度按字节；可含 NUL |
| `OW_BINARY` | 同上 | 任意字节，包括 0 和 0xff |

字段名是非空 NUL 结尾 UTF-8，大小写敏感且逐字节匹配，不做 Unicode 归一化。名称不能表达内嵌 NUL。名称最多 1 MiB，单个字符串/二进制 payload 最多 1 MiB；Record 的逻辑预算是 8 MiB，不等于实际分配器内存。公式和边界见[内部详解](IMPLEMENTATION_INTERNALS.zh-CN.md)。

输入只借用到调用结束，成功存储时复制内容。返回的字符串不是数据库可变内存，也**不保证添加文本终止零**。打印时使用长度控制或自己补零；不要对返回字节无条件 `strlen`。生命周期示例中的 `{'a',0,0xe4,0xb8,0xad}` 是五字节 UTF-8 字符串，不能用 `strlen` 代替 `sizeof`。

从 get 取得的值调用 `ow_value_release(&value)`；数组调用 `ow_array_release(array)`。释放函数接受 NULL；值释放后重置为空值。不要释放借用输入，不要复制一个拥有返回缓冲的 OwValue 后对两个副本分别释放，也不要用不匹配的 CRT `free`。重用输出变量前先释放旧结果，否则函数把输出清零会丢失旧指针。

## 6. 排列与引用删除

`OW_FRONT`/`OW_BACK` 的 anchor 必须是零。`OW_BEFORE`/`OW_AFTER` 必须提供当前存活且属于目标 Collection 的 Occurrence；缺失返回 NOT_FOUND，别的 Collection 返回 INVALID。插入时同时给 Record ID 与 Collection ID；成功后这两个关联不再改变。

成员读取返回 **Occurrence ID** 数组。要知道每项的 Record，用 `ow_occurrence_get`；不能把成员 ID 当 Record ID。比较只接受同 Collection 的两次出现；结果正负代表动态顺序，不是 ID 数值大小。

移到自己之前/之后、已有正确端点或已有紧邻关系，成功但不改变 Revision。移动不改变 ID；删除后该 ID 不再存活，成功分配过的 ID 不复用。跨 Collection 转移不在原子操作范围；不能把两次独立操作描述成事务。

删除 Record 前必须移除它的全部 Occurrences，删除 Collection 前必须清空它。失败返回 `OW_CONFLICT`，不会暗中级联删除。生命周期程序在备份之后逐项移除四次出现，再删除空 Collection 与 Record；完整数据库销毁则有内部统一释放顺序。

## 7. Revision 与相等查询

一个成功且真正改变逻辑数据的公开 mutation 增加一次 Revision。失败、读取、相等字段覆盖、移到已满足位置、删除已有 Record 上的缺失字段都不增加。删除不存在对象通常是 NOT_FOUND；`ow_record_exists` 对非零缺失 ID 则 OK/false。持久化 no-op 没有 WAL 帧，但仍可能在编码准备阶段因 OOM 失败，不能说 no-op 调用绝不分配。

检查点、策略设置、备份和维护不产生新的数据 Revision。Revision 不是时间，不是数据库 ID，也不表示支持多操作事务。`ow_query_equal` 以严格类型相等扫描 Record，结果按 Record ID 递增；缺失字段不等于 Null，整数 1 不等于浮点 1.0。返回数组与数据库独立。

## 8. 检查点与历史视图

先 `ow_checkpoint_create`，再 `ow_checkpoint_info` 查询本会话实际选择的 generation/Revision。它不增加数据 Revision。检查点保存完整对象图、已删除 ID 的高水位与原生 LKS 顺序；不是只保存活对象数量。默认保留 2 代，可设置 1..3；设置只影响后续发布，并随下一份选择元数据持久化。

已经清理历史的库还必须保留历史基线，它占一个内部代数槽。只有一个槽位时，新的当前检查点或需要新基线的清理会保守拒绝 `OW_RETENTION_UNSUPPORTED`。不要把“设置 1 成功”解释成“后续全部维护必定可执行”。损坏孤儿文件和未发布临时文件也不等于受保证的保留代。

历史调用 `ow_history_open(db, saved_revision, &view)`，然后通过 `ow_history_*` 查询，不存在历史写接口。视图拥有独立重建状态，源库后来改变或关闭不改变它。返回副本可在视图关闭后继续使用，最后仍需释放。若 requested Revision 晚于当前或早于已保留基线，返回 HISTORY_UNAVAILABLE；未裁剪的库支持 Revision 0。

历史构建成本可能很高：即使读取很早的 Revision，仍校验完整保留链及语义，不能因为目标早就忽略后面的已提交损坏。历史接口不截断尾部；需要恢复的不完整写入会报 RECOVERY_REQUIRED，而不是偷偷修复。

## 9. 保留、清理与验证

`OW_RETAIN_MANUAL` 是默认；Manual 与 Forever 的 `last_n` 为 0。Last N 要求正数，至少保留最近 N 个已提交 Revision（含当前）。设置策略本身不删文件；不能要求已经裁剪掉的旧范围重新出现，过大的扩展请求可返回 HISTORY_UNAVAILABLE。

`ow_history_range` 的 `[earliest_revision,current_revision]` 是包含两端的保证范围。`reclaimable` 只表示存在可安全回收的完整旧段且槽位足够，不是将回收的字节数。64 MiB 分段意味着小库即使 Last N=3，也可能继续保证从 0 开始的全部状态。

显式 `ow_history_cleanup` 先验证、重建段边界前的基线、确认新基线加后缀等于当前状态，发布两份恢复根之后才删旧段；绝不是“有检查点就可删 WAL”。Manual/Forever 下不会推进基线，但可完成上一次已授权、已不在保证范围内的前缀残留删除。具体状态见[恢复根规范](HISTORY.md)。

`ow_verify_database` 读取真实持久化内容并重建当前与最早状态，检查根/备份根与选择元数据一致性；它不自动修复或删坏文件。普通 writable open 可恢复而 verify 仍报元数据缺副本/不一致，是有意区分。失败先看状态和文件，不要直接手删所谓“多余”基线。

## 10. 完整备份与不确定结果

用 `ow_backup_create(db, absolute_new_destination)` 创建完整副本。目标不得已存在或位于源库内部，父目录必须存在。完成前带 `backup.incomplete`，普通 open 拒绝；复制并刷盘、独立打开比较/验证成功后，才将标记改为 `backup.complete`。失败目标可能留存，不能当作完成备份。重新备份选择新的目标；不要覆盖故障证据。

完整备份可独立 writable open，保留相同 Database ID、当前 Revision、字段/顺序/ID 高水位与保证历史范围，不依赖源路径或源锁。同盘备份不防整盘失效；尚无增量、云备份或损坏历史修复能力。

`OW_COMMIT_UNKNOWN` 表示内存已经应用，但日志写入/刷盘/轮换等结果不能向调用方确认为成功。本次输出 ID 保持零；不能用零推断操作没提交。实例进入隔离，后续普通访问返回 RECOVERY_REQUIRED。关闭并 validated reopen，根据恢复后的 Revision 与业务状态判断，不盲目再次 create/insert。

维护发布失败也可能已经使新元数据可见。RECOVERY_REQUIRED 要先重开；cleanup 的 IO 还可能出现在新根发布成功后的删除阶段，不表示整个维护回滚。close 返回 RECOVERY_REQUIRED 时对象已经释放；不要再对同一个指针调用 close。BUSY 是不消耗对象的例外。

## 11. 排障清单

| 现象 | 先检查 | 不应采取的动作 |
|---|---|---|
| 创建返回 CONFLICT | 目标是否已存在，包括空目录 | 覆盖旧目录 |
| open 返回 BUSY | 是否另有会话或进程持锁 | 删除活锁文件 |
| get 返回 NOT_FOUND | Record/字段是否存在、历史 Revision 是否足够晚 | 将缺失字段当 Null |
| 插入/移动 INVALID | placement 与 anchor 是否符合所属 Collection | 用失效 ID 或别库 ID 猜测 |
| verify 报损坏 | 保留原始字节，定位根/段/检查点和 CI oracle | 重算摘要后强行跳过 |
| 历史读很慢/内存增长 | 全链审计、视图全量重建、多个副本共存 | 宣称只扫描尾部 |
| 检查点 CAPACITY | 完整文件 256 MiB 上限，非数据库总容量上限 | 调整未文档化的 wire 常量 |
| Last Days 不可用 | WAL 无持久时间戳 | 从文件 mtime 编造提交时间 |

完整示例可以复现 API 用法，不替代系统故障处理策略。继续阅读[验证与证据](IMPLEMENTATION_VALIDATION.zh-CN.md)、[内部机制](IMPLEMENTATION_INTERNALS.zh-CN.md)或[交接入口](HANDOFF.md)。

## 12. 显式字段缓存与有序范围

v1.1.0 的48函数保留原有44函数，新增显式有界索引启停及 live/history 类型范围查询。索引是会话级可选缓存；维护失败释放配置，显式再次启用前一直扫描。结果为按值及永久ID排序的独立数组。条目和复制键字节预算不是总内存上限。详见[完整契约](IMPLEMENTATION_API_REFERENCE.zh-CN.md#preview4-step15)。

```c
OwRecordId *ids = NULL;
size_t count = 0;
OwValue lower = {.type = OW_INT, .as.integer = 10};
OwValue upper = {.type = OW_INT, .as.integer = 20};
OwStatus status = ow_field_index_enable(db, "score", 10000, 1024 * 1024);
if (status == OW_OK) {
    status = ow_query_range(db, "score", OW_INT, &lower, 1, &upper, 0,
                            &ids, &count);
    /* Consume ids only when status == OW_OK. */
    ow_array_release(ids);
    ow_field_index_disable(db);
}
```

未启用索引也可以直接调用ow_query_range；enable不是查询前置条件。需要的正预算不足时，应用可记录错误并继续扫描查询；不要把enable失败当作数据不完整。禁用不会释放已经返回的ID数组。history范围同样使用OwValue界限，但通过已有OwHistory读取，不能为历史view启用live缓存。
