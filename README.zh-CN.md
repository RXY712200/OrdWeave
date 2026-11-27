# OrdWeave

[English](README.md)

OrdWeave 是面向 **Windows x64 的 C17 嵌入式静态数据库**，使用独立维护的 LayerKeySort Full 管理顺序和ID索引。它把“共享的数据”和“某次出现的位置”分开，适合需要重复成员、独立移动和历史恢复的有序清单、图层成员或编排数据。


v1.1.0 技术基线：48个公开函数、Full4.2、压缩永久Occurrence ID索引、显式字段索引和类型范围查询。[实际发布状态及源码](docs/HANDOFF.md) · [API](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md)。

## 数据模型与能力

Record拥有具名字段：null、bool、int64、有限double、UTF-8字符串、binary。Collection拥有成员顺序；Occurrence表示某Record在某Collection里的一次出现。相同Record可重复加入，每次有独立Occurrence ID；移动某次出现不改Record字段。仍被引用的Record和非空Collection不能删除。

内存数据库不接触磁盘。持久化数据库由Windows锁保证单写者，每次真正改变状态的操作单独增加Revision并写入、flush WAL；不支持多操作事务。已实现native LKS检查点、不可变历史视图、Manual/Forever/Last N策略、显式安全WAL前缀回收、可独立重开的全量备份和只读持久材料校验。只设置策略不会删除文件。

## 构建和最小示例

需要Windows x64、Git递归子模块、CMake 3.21+、C17编译器；启用测试还需Python 3。Visual Studio：

```powershell
# Run in a recursive checkout of the v1.1.0 source revision listed in docs/HANDOFF.md.
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

GCC/MinGW与Ninja使用独立目录并改为 `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`，不要传 `-A x64`。已有clone执行 `git submodule update --init --recursive`。GitHub自动ZIP/tar不包含子模块，不是自包含源码包。按交接记录选择准确源码提交；旧版本标签保留当时的源码与文档。

固定LayerKeySort Full v4.2.0 Stable提交 `f751e1ee2afd682874281d6dc495969f9eca49eb`，仅通过公开API集成，不修改上游源码。

```c
#include "ordweave.h"
int main(void) {
    OwDatabase *db = NULL;
    OwRecordId record = {0};
    OwCollectionId collection = {0};
    OwOccurrenceId first = {0}, second = {0};
    OwValue value = {.type = OW_INT, .as.integer = 42};
    if (ow_database_create(&db) != OW_OK) return 1;
    int failed = ow_record_create(db, &record) != OW_OK ||
        ow_field_set(db, record, "answer", &value) != OW_OK ||
        ow_collection_create(db, &collection) != OW_OK ||
        ow_occurrence_insert(db, collection, record, OW_BACK,
                             (OwOccurrenceId){0}, &first) != OW_OK ||
        ow_occurrence_insert(db, collection, record, OW_FRONT,
                             (OwOccurrenceId){0}, &second) != OW_OK;
    ow_database_destroy(db);
    return failed;
}
```

这里同一Record产生两个不同的Occurrence。完整的 [quickstart.c](docs/examples/quickstart.c) 还检查顺序和返回值所有权；[lifecycle.c](docs/examples/lifecycle.c) 演示全部字段类型、持久化、检查点、历史、策略、回收与备份。

外部CMake工程设置 `BUILD_TESTING=OFF` 后 `add_subdirectory(path/to/OrdWeave ordweave)`，链接 `OrdWeave::ordweave`，只include `ordweave.h`。使用匹配编译器/CRT/packing设置；当前不承诺跨工具链二进制ABI或安装SDK。完整工程见 [使用指南](docs/USAGE.zh-CN.md)。

创建持久化数据库需尚不存在的绝对UTF-8 Windows盘符路径，父目录已存在；用 `ow_database_open` 重开。调用方串行化全部访问。get和查询结果是独立副本，分别由 `ow_value_release`、`ow_array_release` 释放。遇到 `OW_COMMIT_UNKNOWN` 必须停止、关闭重开确认结果，不能看到输出ID为零就盲目重试。

## 按任务阅读

| 目标 | 文档 |
|---|---|
|开始使用|[使用指南](docs/USAGE.zh-CN.md)，[完整C示例](docs/examples/)|
|查询某个函数|[48个公开API（历史44及当前4项扩展）](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md)，[英文概览](docs/API.md)，[权威头文件](include/ordweave.h)|
|理解整个执行过程|[源码执行导读](docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md)，[英文架构概要](docs/ARCHITECTURE.md)|
|修改或调试内部模块|[内部结构与函数依赖](docs/IMPLEMENTATION_INTERNALS.zh-CN.md)，[开发维护规则](docs/DEVELOPMENT.zh-CN.md)|
|重现测试与查看证据|[验证地图](docs/IMPLEMENTATION_VALIDATION.zh-CN.md)，[RC评审记录](docs/RELEASE_CANDIDATE.md)|
|贡献代码或继续维护|[项目说明](AGENTS.md)、[技术交接](docs/HANDOFF.md)和[开发指南](docs/DEVELOPMENT.zh-CN.md)|
|评估成本和后续工作|[技术债登记](docs/TECHNICAL_DEBT.md)，[六阶段完成记录](docs/ROADMAP.md)，[变更记录](CHANGELOG.md)|
|核对磁盘字节布局|[WAL/manifest](docs/PERSISTENCE.md)，[checkpoint/selection](docs/CHECKPOINTS.md)，[history/root/policy/backup](docs/HISTORY.md)|

## 边界与许可

不支持SQL、持久化/自动二级字段索引、网络服务、多写者/并发调用保证、跨Collection原子转移、Linux/macOS、Last Days或增量备份。checkpoint恢复减少普通构造路径的tail，但整体open还会独立语义审计保留WAL，不承诺整体启动加速。多轮重放、完整缓冲/复制、SHA provider初始化等真实成本见技术债登记。进程终止测试不等于物理掉电认证。

OrdWeave使用 [MIT License](LICENSE)；LayerKeySort和Obscura64适配来源与SHA见 [第三方声明](THIRD_PARTY_NOTICES.md)。没有引入ProbSpan代码。




## 字段有序查询

v1.1.0 的48函数保留历史44函数，新增显式有界索引启停及 live/history 类型范围查询。索引是会话级可选缓存；维护失败释放配置，显式再次启用前一直扫描。结果为按值及永久ID排序的独立数组。条目和复制键字节预算不是总内存上限。详见[完整契约](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md#preview4-step15)。

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
