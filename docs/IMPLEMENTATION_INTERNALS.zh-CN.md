# RC.1 内部实现：结构、调用依赖、发布与失败分支

[总导读](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) · [公开API](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [验证](IMPLEMENTATION_VALIDATION.zh-CN.md) · [维护规则](DEVELOPMENT.zh-CN.md)

固定代码：`3f99029da73e3b4f0a75a538dd241cf94cda23d7`。下面的函数是私有实现，不是新公开接口；本次只写文档，不调整生产结构。磁盘偏移仍集中在 [PERSISTENCE.md](PERSISTENCE.md)、[CHECKPOINTS.md](CHECKPOINTS.md)、[HISTORY.md](HISTORY.md)，这里解释这些字段如何被处理，避免产生另一个竞争规范。

## 1. 全部生产文件及编译归属

| 源文件/私有头 | 编译或包含关系 | 责任与关键调用 |
|---|---|---|
|[ordweave.c](../src/ordweave.c)|生产TU 1|Record/Field/Collection/Occurrence/OwDatabase、core操作、统一析构、逻辑比较|
|[lks_adapter.c](../src/lks_adapter.c)|生产TU 2|ID比较/locate/唯一插入、公开Order insert/move分派、LKS状态映射|
|[persistence.c](../src/persistence.c)|生产TU 3|公开ordinary API、open_db、mutate、guard、结果确认|
|[wal_format.c](../src/wal_format.c)|生产TU 4|大端整数、确定性payload、严格decode、ow_apply单一模型派发|
|[wal_windows.c](../src/wal_windows.c)|生产TU 5|OwStore、路径、文件、OS锁、append/flush、rotation、段扫描、open|
|[sha256.c](../src/sha256.c)|生产TU 6|BCrypt双段SHA helper及句柄清理|
|[checkpoint_core.inc](../src/checkpoint_core.inc)|由ordweave.c文本包含|访问私有对象布局；export/import、namespace、resolver|
|[checkpoint_windows.inc](../src/checkpoint_windows.inc)|由wal_windows.c包含|文件重读验证、选择元数据、代数、候选恢复、代清理|
|[history_windows.inc](../src/history_windows.inc)|同一Windows TU中随后包含|历史视图、语义审计、策略/root、WAL前缀、backup/verify|
|[core_names.h](../src/core_names.h)|仅core实现重命名|避免公共包装与core同名定义冲突，不是对外宏API|
|[core_internal.h](../src/core_internal.h)|core跨TU声明|ow_core_*、store attach、validate和same_state契约|
|[lks_adapter.h](../src/lks_adapter.h)|适配器内部协议|OwIndexed首成员布局及索引/order帮助函数|
|[wal_internal.h](../src/wal_internal.h)|包装/格式/Windows/SHA共享|OwCommand、opcode、frame/segment常量、会话函数；测试条件声明|
|[checkpoint_internal.h](../src/checkpoint_internal.h)|core/checkpoint共享|256MiB/144/416常量、export/import/exchange、测试分配边界|

`.inc`在其owner的作用域里能调用前面定义的static函数；例如cp_load_bytes使用Windows TU的open_file，cp_resolve直接看Occurrence。这减少额外导出结构的需求，也使文件间耦合较紧，不能当成各自独立链接模块。不要把同一个inc再列进add_library而产生重复定义。

## 2. 私有数据结构逐字段

| 结构 | 字段与含义 | 不变量 |
|---|---|---|
|OwIndexed|id、index_handle、teardown_next|嵌入Record/Collection/Occurrence首部；比较只读id；teardown_next只用于析构临时链|
|Field|name、OwValue value、next|拥有NUL结尾名字及bytes副本；同Record中名唯一；链顺序不是字段语义|
|Record|key、fields、budget、refs|预算是逻辑编码成本；refs等于活Occurrence总数，不能因重复成员去重|
|Collection|key、order|拥有独立LksOrder；Order借用Occurrence，数量与所属项一致|
|Occurrence|key、record、collection、handle、checkpoint_seen|关联不可变；每个活对象恰有一个Order residence；seen仅导入去重|
|OwDatabase|三个ManagedOrder、rid/cid/oid/revision、store|counter为已成功消耗ID高水位；store=NULL是内存/未发布core|
|OwHistory|db|独占一个独立core；不暴露其指针给写API|
|OwCommand|op、id/collection/record/anchor、place、name/value|输入借用；创建时id可先0，落盘前必须补成功分配ID|

`OwStore`字段按职责分组：

- root/directory/lock/wal：Windows句柄；path：拥有的UTF-16根路径。
- id/digest：持久Database ID与当前append链摘要；segment/offset：当前段号与已确认边界。
- busy/poisoned：调用状态与隔离，非互斥锁。
- scan_revision/checkpoint_revision/anchor_found：扫描到哪、哪些数据已由检查点提供、捕获候选Revision摘要。
- metadata[416]/retention/selected_generation/selected_revision：检查点选择及本会话实际选中结果。
- recovery_root[256]/first_segment/earliest：锚定恢复起点；history_policy/history_n：保留策略。
- validating/readonly_scan/limit_replay/through/semantic_scan/allow_incomplete_tail：控制扫描用途；root_anchor_found捕获根发布时最新digest。

这些flag不能泛化成“验证模式只读”一个开关；下面列出实际组合。

## 3. 重放同一解析器的不同模式

| 用途 | validating | readonly_scan | limit_replay | 是否应用core | 是否可协调最终短尾 |
|---|---|---|---|---|---|
|open结构pass|1|0|通常0|否|此pass不truncate|
|独立语义审计|0|1|先全链，历史目标可第二轮限定|从强制基线后实际应用|仅服务writable open的审计允许排除短最终尾，本身不写|
|open主结果|0|0|0|只应用checkpoint后|可SetEndOfFile+flush|
|历史主结果|0|1|1/through=target|checkpoint之后至target|否；需恢复时报错|
|cp_anchor校验|1|继承|继承|否，只捕获摘要|不能借此忽略完整坏帧|

每次扫描都校验完整保留链的字节，即使只应用某段Revision范围。`scan_revision`表示文件链进度，不是所构造core的Revision；limited replay到target后，后续帧仍解码/验链，但不再修改core。测试计数器有意排除semantic_scan，只统计主结果实际应用的tail；“计数1”不意味着整个打开只执行了一次core mutation。

## 4. core帮助函数与直接依赖

| helper | 谁调用/依赖 | 关键分支与后置条件 |
|---|---|---|
|utf8|name_valid/value_valid|逐码点验证；严格拒绝过长编码、代理项、错误续字节和超范围；binary不调用|
|name_valid|字段set/get/remove/query|非空、长度、UTF-8；不归一化或忽略大小写|
|value_valid/payload/equal|字段操作、query、codec验证|类型决定union和长度；有限float；相等先类型，零符号不区分|
|copy_value/free_fields|set/get、统一析构|只复制bytes类型payload；失败清零新输出，已准备资源由调用者释放|
|find_field|set/get/remove/query/same_state|返回Field**槽位，既可替换/断链，也可在尾部插入|
|capacity|真正变化的core mutation|ID/Revision不能wrap；no-op先识别，饱和Revision仍可成功无变化|
|anchor_get|Occurrence insert/move|端点anchor=0，相对anchor同Collection且活，输出a先NULL|
|stage_occurrences|insert/remove|完整替代ManagedOrder借用旧对象；临时新Handle不覆盖已发布index_handle|
|publish_occurrences|对应Order操作成功后|销毁旧索引，安装新索引，遍历刷新index_handle；无分配|
|detach_index|database_destroy|先串对象临时链，再销毁容器；不需要LKS删除mutation|
|ow_core_exchange|checkpoint_load|交换完整core但保留各自store归属；旧构造core由tmp析构|
|ow_core_same_state|history_same_state|不分配checkpoint buffer，比较逻辑内容/预算/refs/计数器和顺序，忽略跨实例Handle地址|

`ow_core_same_state`按两个ID索引并行遍历Record，按名字在另一个Record里找字段，因此字段插入链顺序不同仍可相等。它随后比较Collection ID与Order中Occurrence ID序列，最后比较每个Occurrence的两个关联。早期发现不符返回0；不是一个hash碰撞概率比较，也没有256MiB总数据库限制。其字段查找可有额外线性成本，不宣称整个函数恒为O(N)。

## 5. Record/Collection与Occurrence的原子性差别

Record create准备对象后只需要一次唯一索引插入；Collection create还准备空Order，失败销毁它。两者在索引成功前都不消费counter。Record delete先检查refs；Collection delete先检查order size；经过容量检查再移除索引，释放已不被借用的对象。

Occurrence insert/remove要维持两处LKS结构，采用prepare-then-publish替代全局索引。准备阶段失败直接销毁替代索引，不需要恢复旧Handle或回滚旧索引的内部Revision。Order操作成功才切换索引和引用数。若改成“先删索引再插Order失败就反向insert”，就必须处理恢复分配失败和不同Handle，不能称等价重构。

move预判a==o、端点及lks_order_next/previous邻接后再检查Revision容量，真正move交给LKS公开API；不触及global ID索引。跨Collection没有此原子路径。

## 6. 值准备、编码准备与磁盘提交

`mutate`并不在每种持久输入上先调用完整core验证，而是先 `ow_encode`验证可序列化字段并分配帧，再core应用。因而可合法返回NOMEM而尚未查出缺失对象；API不规定所有非法条件的优先状态。

`ow_encode`为整帧calloc，payload位于偏移96，但此时header/SHA尚未最终填写。字段复用规范编码；Int转换成uint64再写补码，解码时对超过INT64_MAX的值用安全负数重构表达式，避免依赖大无符号转有符号的实现行为。浮点通过memcpy取bits再显式BE，不dump整个union；只有零先变正零。字符串含NUL仍由长度保留。

`ow_decode`先清command/value/name输出，检查op/id和逐层剩余长度后再分配name。借用payload里的value bytes由core setter复制，调用者最终free(name)和frame。即使decode在分配name后报错，replay和import的done路径仍会释放它；不要将它当作成功才存在的资源。

`ow_apply`是九种opcode的单一派发：create使用core真实分配ID，并与日志所给ID比较；非创建操作使用永久ID调用core；没有从checkpoint/backup走到另一个字段存储引擎。

## 7. WAL与Windows帮助函数

| helper | 调用入口 | 输入、验证、失败/清理 |
|---|---|---|
|device/safe_path|open/create、backup|UTF-8严格转换、绝对路径/组件规则；拥有返回path，失败free。非递归mkdir|
|child|文件/目录操作|拼接已检查根和内部suffix，分配独立wide string；调用者free|
|open_file|各格式文件I/O|CREATE_NEW/OPEN_EXISTING等；最终reparse/目录/多硬链拒绝。只读打开额外share write支持审计独立句柄，写句柄不share write/delete|
|write_exact/read_exact|WAL/元数据/backup|处理部分I/O；read提前EOF是CORRUPT，Win32失败IO；不吞掉零写|
|seek/file_size|replay/append/验证|显式64位位置与负长拒绝；调用者根据失败返回IO|
|digest_check|定长头/元数据|末32字节与前面SHA比较；版本检查通常由上层先执行|
|io_status|Windows错误分类|共享/锁冲突BUSY、缺文件/路径NOT_FOUND，其余IO；并非所有路径统一用它|
|new_segment|ow_store_commit|容量→header→pending write/flush→rename→新handle/offset；失败时commit层隔离|
|discover|open、history、boundary|严格文件名/序号，返回最高保留段号；legacy从1连续，root从first连续，不能把返回值当段个数|
|clear_pending|成功open尾部|只删除已验证链最后下一号pending；其它冲突是损坏，不任意扫描选择新genesis|
|ow_store_guard/enter/leave|公共dispatch/检查点|控制busy/poisoned，不建立线程happens-before|

持久化create先拿writer.lock，然后准备wal目录与首段，manifest最后经pending刷盘/rename发布。失败目录可能仍存在，但open不会再次初始化它。existing open也验证backup.incomplete；GetFileAttributes硬错误不能当作marker不存在。最后只有完整结构/语义/结果检查完成，open_db才attach store并发布out。

`ow_store_commit`在实际core改变后补Revision、ID、prev digest与两个SHA，轮换后精确append/flush。成功才更新store digest/offset。错误保留未知结果，poisoned防后续append遮盖坏尾。frame buffer归mutate统一free；store持有文件句柄直到close或扫描替代关闭。

## 8. replay_segment逐层消除不可信输入

1. 读128字节段头，验证magic/version/SHA、身份、sequence、expected首Revision和前驱digest；未知段reserved是UNSUPPORTED。
2. 剩余不足96时只检查可见magic前缀；完整header先检查version和header SHA，再取N并检查16MiB及payload长度/保留零。
3. 不足完整N只进入tail分支，不按伪造长度无限分配。完整frame分配N，读body，验证完整SHA、COMMIT03、prev digest。
4. expected Revision必须为scan_revision+1；decode规范payload。按模式决定是否ow_apply，实际应用时core Revision必须恰好expected，no-op frame不可过关。
5. 更新scan_revision、指定checkpoint/root的anchor_found与当前digest，释放frame。
6. 失败统一关闭本次非转移句柄、free(frame)。最终成功把最后段句柄/offset交回store。tail分支根据last/readonly/allow flags决定拒绝或截断；完整坏帧不进入可恢复分支。

结构扫描不等于语义重放。Phase6修正通过 `history_semantic_state` 分配独立core从基线验证全部操作，再比较主结果；其控制路径见总导读。此修正已在 `15d14dc85187a1ee18b252c281511e1059e49b55` 提交，与本次纯文档工作不同。

## 9. Checkpoint core缓冲与恢复

`CpWriter`持p/n/cap/status：每次cp_write先检查当前错误，再检查最大256MiB剩余；几何扩容失败保留旧缓冲归writer所有。export最后补完整长度、算总SHA再追加。错误统一free(w.p)。Record字段排序只分配临时Field*数组，不重排live链；snapshot buffer和Snapshot每Collection及时销毁，最终总文件仍完整留在内存。

`CpReader`持p/n/status；cp_read只在n足够时推进，否则锁住CORRUPT状态。固定header校验总长/ID/SHA/计数、高水位总和不得超过Revision；逐Record ID递增，逐字段名递增；逐Collection记录blob借用原输入；逐Occurrence验证id与关联。

导入为了复用core，临时把counter设为要导入ID减一，用普通core create生成该ID。导入中Revision是构造活动计数，结束前确认不超过记录Revision，然后覆盖为真实已提交值；不能把中途Revision公开。所有对象建完后 `cp_resolve` 才有完整索引可查，seen去重防止同对象重复出现在一张或别的集合快照中。

每个collection的native restore成功后替换它的构造Order。此时旧构造Handle全部失效，但紧接着仅遍历新Order装回handle，不读取旧地址。若后一个集合restore失败，统一core destroy仍能销毁已换与未换的两种Order。blob视图只在导入调用内借用，不带到成功core里。

## 10. Checkpoint Windows帮助函数族

| helper | 直接上层/依赖 | 分支与结果 |
|---|---|---|
|cp_load_bytes|meta/import/backup/root|先file_size检查32..limit，再分配/read；不盲信长度；成功buffer由调用者free|
|cp_meta_validate|cp_meta_read/verify|416字节、OWSELECT、版本/身份/SHA、count<=keep<=3、严格代号递减、Revision非递增、未用槽零|
|cp_verify_file|cp_publish|先字节完全相等；checkpoint再建tmp core导入，selection做meta_validate；统一free/tmp destroy|
|cp_publish|checkpoint/root selection|metadata临时名加随机数；写/刷/重开验证，rename后再验证；published决定能否删除临时及是否poison|
|cp_anchor|checkpoint_load|重置到正确genesis/root起点，完整扫描并捕获目标Revision digest，不能仅信selection里写了某个anchor|
|checkpoint_load|open/history|两候选版本/硬错误分类，校验各anchor；按代序试可用且不晚于through的候选；tmp import成功才exchange|
|cp_next_generation|checkpoint/cleanup|扫描规范最终与pending名字、比较选择高水位，max+1并检查wrap；不是恢复选择算法|
|cp_cleanup|发布后|只处理旧selection引用且新selection不再引用的代；保护root baseline；缺失文件可容忍|
|cp_sweep|发布后补扫|要求current/bak相同；验证旧未引用文件能import后再删；损坏/unsupported孤儿保留|

开库候选回退可以容忍缺失/损坏普通checkpoint，但不能吞掉UNSUPPORTED/IO/NOMEM。verify会额外要求选择对完整一致，逐个导入所引用的快照。初始全WAL路径没有选择也有效。根基线是必需材料，不能按普通可丢加速artifact处理。

## 11. 历史/语义审计帮助函数族

`history_scan_reset`把scan_revision设为earliest，digest设为root predecessor或genesis零。历史扫描复制OwStore的参数，但不拥有live根/锁/path；只把自己的wal设INVALID_HANDLE_VALUE并自己关闭。不要对浅拷贝调用ow_store_close，否则会错误关闭原会话共享资源；当前路径只用history_scan_close关闭本次wal。

`history_semantic_state`第一遍从基线到最新实际应用所有保留mutation。若target较早，销毁第一份core，再从基线重建目标范围；后续帧仍进行结构检验。`semantic_scan`只影响测试计数分类，不降低校验。两遍都有done释放，返回out只在完整成功后交出。

`history_reconstruct_impl`先读manifest和root/策略（proposed分支使用已提供根），检查目标边界并扫描全链；得到audited目标core，再独立恢复适用检查点/尾，校验target Revision及same_state。最后history_open分配小OwHistory包装；最后这一步也可能OOM，已建core会销毁。

这些步骤解释了为什么一个历史查询可能同时持有多个完整core、native snapshots、frame buffer。不能把“历史view只存一个指针”误当成整体内存开销很小。

## 12. 根、策略与安全删除帮助函数族

- `history_root_validate/read_root`：256字节、身份、版本、SHA、epoch/gen/seq非零、R+1、C>=R、policy/N一致及reserved；不认证敌对重写。
- `history_policy_load`：独立96字节策略优先于root内旧配置；缺失才用root/default，坏/未来策略不忽略。
- `history_root_load`：都不存在则legacy；任一非缺失硬错误直接失败；同epoch不同字节损坏；较新epoch不得倒退R/C，随后真实baseline/suffix仍须验证。
- `history_baseline`：先确认suffix到达root的C/digest，再载named checkpoint，验证gen/R/SHA/anchor并导入；缺必需checkpoint为RECOVERY_REQUIRED，不fallback已删genesis。
- `history_boundary`：遍历header，选择满足 `start-1 <= floor` 的最新完整段，以start-1与前驱digest为候选；前置verify提供全链语义依据。
- `history_selection`：把两份选择收敛到新baseline；之后才有root发布，不允许旧checkpoint仍凭已删祖先获权威。
- `history_publish`：随机临时→write/flush→reopen相等→replace/write-through→再验字节。rename已发生后的错误poison；只删除自己未发布的临时。
- `history_delete_prefix`：先重读两根并与会话root逐字节相等，才删除oldfirst..first_segment-1，缺失文件可重试；不重编号后缀。

`ow_history_cleanup`在已有根场景会先重建/比较，再协调根副本，随后verify；不能把它描述为一个自动在open中跑的compactor。需要推进时才分配新baseline和epoch、做proposed恢复与比较，再发布selection和root对。任一步失败都不以“删除旧文件省空间”为恢复办法。Manual/Forever也可以完成已经不在保证范围的旧前缀残留，但不推进earliest。

## 13. Backup与verify的真实边界

`history_backup_parent`将目标父与源root句柄映射为volume-GUID规范路径，拒绝源内嵌套；词法前缀检查先筛普通情况。`history_copy_directory`仅接受数字序号+`.wal`/`.cp`，规范pending跳过，未知/长名字拒绝再格式化。`history_copy`创建新文件，用64KiB缓冲循环ReadFile/write_exact并flush，失败释放buffer/句柄。

备份的manifest最后复制，但验证前已经可能存在；`backup.incomplete`才是普通open拒绝的门。私有allow_incomplete仅用于验证新副本。验证成功关闭副本session后才rename completion marker；失败可能留下目标和标记，不保证删除整个目标。实现没有把源锁句柄或source path写进备份。

`ow_verify_database`先guard；有root时要求两根有效且相同；history_reconstruct验证当前；然后要求checkpoint选择对同存/同字节，逐项检查digest与import；最后history_range重建earliest。它不像普通open那样为了可用性吞掉坏的普通checkpoint；也不会做pending清理、尾截断、根协调或孤儿删除。验证成功仍不是对所有未引用孤儿文件的全面磁盘取证。

## 14. 安全性质、成本与证据不能混写

| 分类 | 例子 | 怎么处理 |
|---|---|---|
|必须正确|引用数、ID不复用、已确认提交恢复、root前缀删除边界|错误要复现并独立审查，不能登记为可接受性能债|
|已接受成本|global index重建、多遍语义审计、完整checkpoint buffer、每次BCrypt provider|记录触发条件与证据，见TECHNICAL_DEBT.md|
|证据缺口|实际设备power-loss、完整系统IO错误组合|明示未验证，不能从TerminateProcess推导|
|产品范围外|SQL、多写者、Last Days、Linux/macOS支持|不是自动必还的债务|

本次仅文档化现有机制。对动机中未由提交/注释明确陈述的解释，应理解为根据准备/发布结构得出的工程解读，不冒充作者过去作过的设计决议。继续定位每条行为的断言见[验证指南](IMPLEMENTATION_VALIDATION.zh-CN.md)，后续修改规约见[开发指南](DEVELOPMENT.zh-CN.md)。
