# OrdWeave

[English](README.md)

OrdWeave 是面向 **Windows x64 的 C17 嵌入式静态数据库**，使用独立维护的 LayerKeySort Full 管理顺序和ID索引。它把“共享的数据”和“某次出现的位置”分开，适合需要重复成员、独立移动和历史恢复的有序清单、图层成员或编排数据。

[**v1.0.0-rc.1**](https://github.com/RXY712200/OrdWeave/releases/tag/v1.0.0-rc.1) 已作为 GitHub **Prerelease** 发布。六个实现阶段均已通过独立评审，**Stable 尚未获准或发布**。RC代码基线为 `3f99029da73e3b4f0a75a538dd241cf94cda23d7`。当前main新增的完整中文指南属于发布后文档，不在不可变RC标签内。

## 数据模型与能力

Record拥有具名字段：null、bool、int64、有限double、UTF-8字符串、binary。Collection拥有成员顺序；Occurrence表示某Record在某Collection里的一次出现。相同Record可重复加入，每次有独立Occurrence ID；移动某次出现不改Record字段。仍被引用的Record和非空Collection不能删除。

内存数据库不接触磁盘。持久化数据库由Windows锁保证单写者，每次真正改变状态的操作单独增加Revision并写入、flush WAL；不支持多操作事务。已实现native LKS检查点、不可变历史视图、Manual/Forever/Last N策略、显式安全WAL前缀回收、可独立重开的全量备份和只读持久材料校验。只设置策略不会删除文件。

## 构建和最小示例

需要Windows x64、Git递归子模块、CMake 3.21+、C17编译器；启用测试还需Python 3。Visual Studio：

```powershell
git clone --recurse-submodules --branch v1.0.0-rc.1 https://github.com/RXY712200/OrdWeave.git
cd OrdWeave
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

GCC/MinGW与Ninja使用独立目录并改为 `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`，不要传 `-A x64`。已有clone执行 `git submodule update --init --recursive`。GitHub自动ZIP/tar不包含子模块，不是自包含源码包。需要本套新文档时阅读main；克隆RC标签获得的是原发布状态。

固定LayerKeySort Full v4.1.0 Stable提交 `70d7f0ef5857b54381b863d292a213909d87459f`，仅通过公开API集成，不修改上游源码。

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
|查询某个函数|[44个公开API及类型/错误契约](docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md)，[英文概览](docs/API.md)，[权威头文件](include/ordweave.h)|
|理解整个执行过程|[源码执行导读](docs/IMPLEMENTATION_WALKTHROUGH.zh-CN.md)，[英文架构概要](docs/ARCHITECTURE.md)|
|修改或调试内部模块|[内部结构与函数依赖](docs/IMPLEMENTATION_INTERNALS.zh-CN.md)，[开发维护规则](docs/DEVELOPMENT.zh-CN.md)|
|重现测试与查看证据|[验证地图](docs/IMPLEMENTATION_VALIDATION.zh-CN.md)，[RC评审记录](docs/RELEASE_CANDIDATE.md)|
|项目流程或AI接手|先完整读[WORKFLOW.md](docs/WORKFLOW.md)，再读[HANDOFF.md](docs/HANDOFF.md)；编码代理另读[AGENTS.md](AGENTS.md)|
|评估成本和后续工作|[技术债登记](docs/TECHNICAL_DEBT.md)，[六阶段完成记录](docs/ROADMAP.md)，[变更记录](CHANGELOG.md)|
|核对磁盘字节布局|[WAL/manifest](docs/PERSISTENCE.md)，[checkpoint/selection](docs/CHECKPOINTS.md)，[history/root/policy/backup](docs/HISTORY.md)|

## 边界与许可

不支持SQL、二级字段索引、网络服务、多写者/并发调用保证、跨Collection原子转移、Linux/macOS、Last Days或增量备份。checkpoint恢复减少普通构造路径的tail，但整体open还会独立语义审计保留WAL，不承诺整体启动加速。索引重建、多轮重放、完整缓冲/复制、SHA provider初始化等真实成本见技术债登记。进程终止测试不等于物理掉电认证。

OrdWeave使用 [MIT License](LICENSE)；LayerKeySort和Obscura64适配来源与SHA见 [第三方声明](THIRD_PARTY_NOTICES.md)。没有引入ProbSpan代码。
