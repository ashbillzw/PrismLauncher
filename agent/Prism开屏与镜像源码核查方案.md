# Prism 开屏与自托管镜像：源码核查及修改方案

日期：2026-10-28。核查基线：本地 `develop`，HEAD `3d4fc4528`。本文件记录实施前的静态核查；后续实现和验证见 [开屏实现与验证](开屏实现与验证.md)。工作区原有《启动器初步计划.md》修改保留。

## 结论

新增开屏需要少量 UI 与流程接线代码。镜像功能已有一部分完整配置能力；若服务端提供 Prism 兼容元数据，并把元数据内的文件 URL 改为镜像地址，客户端的下载逻辑可能无需改动。应先验证这条配置方案，再决定是否增加 URL 映射代码。

不能只把 `MetaURLOverride` 改成普通 Minecraft 镜像根目录，也不能只复制原始元数据而保留其中所有官方 URL，然后认为所有下载已走镜像。

## 1. 当前已有配置

配置位于全局 API 设置页，源码为 `launcher/ui/pages/global/APIPage.ui`、`APIPage.cpp:144–199`；注册处为 `launcher/Application.cpp:884–890`。

| 配置 | 实际覆盖 | 限制 |
| --- | --- | --- |
| `MetaURLOverride` | Prism 组件元数据根地址 | 要符合 Prism meta 格式；不自动重写 JSON 内部绝对 URL |
| `ResourceURLOverride` | Minecraft 资源对象（声音、材质等） | 路径为 `<根地址>/<hash前两位>/<hash>`；不覆盖资源索引 JSON |
| `LegacyFMLLibsURLOverride` | 旧版 FML 库 | 设置页明确为 Minecraft 1.5.2 及以前；不是现代 Forge 通用镜像开关 |
| `Launcher_META_URL` | 构建时默认元数据根地址 | CMake 缓存变量；运行时 MetaURLOverride 优先 |

证据：`meta/BaseEntity.cpp:58–65`、`minecraft/update/AssetUpdateTask.cpp:115`、`minecraft/AssetsUtils.cpp:288–297`、`CMakeLists.txt:184,205`。

设置页自动补末尾 `/`，并把非 localhost 的 HTTP 地址改为 HTTPS。手工预置时也应使用正确的 HTTPS 根地址。上述覆盖项是全局设置；若要求仅新岛实例使用镜像，需要额外增加实例范围控制，不能直接宣称现有配置支持。

未在本次所查路径发现现代 Forge、Minecraft client JAR、普通 Maven 库各自的通用镜像开关。`BuildConfig.h:176–177` 的资源与库默认地址也不是能覆盖所有绝对下载 URL 的总开关。

## 2. 实际安装与下载路径

```text
开屏下载按钮
  -> InstanceImportTask(整合包 URL)
  -> InstanceList::wrapInstanceTask（暂存、提交实例）
  -> 对 mrpack：ModrinthCreationTask
  -> 设置 net.minecraft / net.minecraftforge 组件版本、下载包内列出的文件
  -> InstanceTask::downloadFiles（由设置决定是否预下载游戏文件）
  -> MinecraftLoadAndCheck / 组件元数据解析
  -> MinecraftInstance::createUpdateTask
       -> LibrariesTask -> Library::getDownloads
       -> LegacyFMLLibrariesTask
       -> AssetUpdateTask
```

关键源码：`InstanceImportTask.cpp:64,95,379`、`InstanceList.cpp:1033`、`modplatform/modrinth/ModrinthInstanceCreationTask.cpp:220,232,574`、`InstanceTask.cpp` 的 `downloadFiles`、`minecraft/MinecraftInstance.cpp:1132`。

- `DownloadGameFilesDuringInstanceCreation` 决定导入期间是否预下载游戏文件。若没有预下载，正常在线启动时仍会执行资源准备（`MinecraftInstance.cpp:1213` 附近）。开屏应明确预下载策略，避免用户以为已完成下载而首次启动又开始大量下载。
- Minecraft 主 JAR、依赖库、原生库及 `mavenFiles` 汇入 `LibrariesTask.cpp` 的下载池；主 JAR 从 `mainJar` 或 `downloads.client` 构造（`OneSixVersionFormat.cpp:228–250`）。
- `Library.cpp:130–224` 优先使用下载对象里的绝对 URL；也可能使用 absolute URL、库的 repository URL，最后才用默认库根地址。改一个默认域名不能覆盖全部分支。
- 资源索引使用元数据里的 `assets->url`（`AssetUpdateTask.cpp:25–36`）；资源对象才使用 `ResourceURLOverride`。
- Forge 组件由 Prism 元数据参与解析。本轮确认了组件与统一库下载路径，尚未抓取目标 Forge 版本的实际元数据与完整网络请求；最终覆盖清单需用目标版本冷缓存安装验证，不能承诺仅列出的域名已穷尽所有请求。
- 模组自身仍按 mrpack 的下载列表处理；Java 获取和微软认证也是独立范围，本轮不把它们自动改走镜像。

## 3. 优先方案：客户端用现有配置，服务器维护镜像元数据

### 服务器侧

建议提供以下逻辑目录（路径名仅为设计示例）：

```text
/prism-meta/v1/index.json
/prism-meta/v1/<组件UID>/index.json
/prism-meta/v1/<组件UID>/<版本>.json
/assets/<hash前两位>/<hash>
/files/<上游host>/<原路径>
```

1. 保持 Prism 元数据结构，覆盖新岛所用 Minecraft、Forge 及其依赖组件闭包。
2. 将其中客户端 JAR、库、Maven 文件、资源索引等 URL 指向自托管文件地址；对于没有显式 URL 的库，也要生成正确的仓库地址，避免回落到官方默认库源。
3. 二进制文件原样镜像，保留原内容哈希。资源索引若仅改下载位置而内容不变，其 SHA-1 也不变。
4. 修改元数据 JSON 会改变其 SHA-256，因此要重新计算其父版本列表中记录的哈希，以及根索引引用的列表哈希；不能删除校验来规避问题。证据：`meta/JsonFormat.cpp:44,69` 与 `meta/BaseEntity.cpp:115–160`。
5. 如果希望原 Prism 页面仍可创建其他版本实例，镜像元数据需覆盖这些版本或有服务端代理策略。只镜像新岛所需组件会影响全局版本列表，这是需要明确的产品取舍。

这部分可用生成脚本产出静态文件交给 copyparty 提供下载；若需要缓存未镜像的任意资源，则是额外的服务端代理能力，不能假定 copyparty 静态目录已经具备。

### 客户端侧

示例配置（占位地址不可直接使用）：

```ini
[General]
MetaURLOverride=https://mirror.example/prism-meta/v1/
ResourceURLOverride=https://mirror.example/assets/
DownloadGameFilesDuringInstanceCreation=true
```

先在隔离配置目录手动验证。通过后可随发行物预置配置，或由首次初始化代码在对应字段为空时填入默认值；不在每次启动时覆盖用户修改。不会额外添加一个“总镜像开关”来替代已有能力。

**这是优先验证方案，尚未部署或实测通过。** 它把 URL 重写和元数据同步工作放在服务器侧，客户端继续使用原任务、缓存和校验。

## 4. 若现有配置方案不适合：最小客户端补丁

仅当不希望维护改写元数据、需要实例级镜像，或目标版本实测发现遗漏时，采用这一补充方案。

- 新增一个集中 URL 映射辅助模块（拟名 `minecraft/DownloadMirror.h/.cpp`），按明确的源主机／路径规则映射到镜像；配置为空原样返回。
- 在 `Library::getDownloads` 创建请求前映射 URL，覆盖主 JAR、普通库、原生库与 Maven 文件的统一入口。
- 在 `AssetUpdateTask::executeTask` 创建资源索引请求前映射 URL；资源对象继续复用已有 `ResourceURLOverride`。
- 元数据继续使用已有 `MetaURLOverride`，可镜像未改写的原始 meta 数据；不要新增重复设置。
- 注册缺失的映射配置并提供必要 UI；若仅作用于新岛实例，需要把作用域从 `LibrariesTask` 等上层传入，不能在全局网络层偷换所有请求。
- 保留文件存储键、缓存位置和原校验值；不重写本地文件 URL、登录、账号或无关模组接口。
- 回退官方源是否允许尚未确定；首版可以明确报错，不默默绕过镜像导致速度不可预测。若实现回退，需与重试／校验状态一起设计。

不建议直接修改通用 `Net::Request`／`ApiRequest` 来替换所有网络地址，其影响会扩散到认证、模组 API 和其他实例。

## 5. 开屏的具体修改位置

拟新增两个模块，名称可在实现时调整：

| 文件 | 职责 |
| --- | --- |
| `launcher/ui/NiijimaHomeWindow.h/.cpp/.ui` | 界面、按钮、状态与进度展示 |
| `launcher/niijima/NiijimaInstanceController.h/.cpp` | 查找目标实例、创建导入任务、记录成功安装的实例 ID、请求启动 |
| `launcher/Application.h/.cpp` | 普通启动展示开屏；保留命令行直接启动、外部导入和原主页入口；窗口生命周期 |
| `launcher/CMakeLists.txt` 及资源清单 | 注册新增源码、UI、字体与图片 |

### 接线方式

- **启动入口**：改 `Application::performMainStartupAction` 的普通启动分支（约 1413 行），而非粗暴替换所有 `showMainWindow()` 调用。命令行实例启动、文件关联导入、二次进程消息要保留原行为。
- **原主页**：开屏「进入 Prism」调用 `Application::showMainWindow()`。该方法已有单例窗口处理，不复制整个 MainWindow。
- **下载**：构造 `InstanceImportTask`，设置实例名称／图标／分组，再用 `instances()->wrapInstanceTask()` 执行；参照 `MainWindow::instanceFromInstanceTask`，复用暂存和最终提交机制，禁止自己解压到正式实例目录后就宣布完成。
- **识别实例**：建议给新岛实例记录专用标识，并在全局保存首选实例 ID；路径／实例被删除时重新扫描标识。获取 staging 最终实例 ID 的通知接口需实现时核对，必要时小幅补充完成信号，不靠名称或“列表最后一个”猜测。
- **启动**：调用 `Application::launch(instance)`，保留 `LaunchController`。`LaunchController.cpp:80–125` 已有无有效账号时询问打开账号管理器、以及无默认账号时选择账号的处理；它不是无条件自动完成登录。
- **任务展示**：订阅现有 `Task::status/progress/stepProgress`，沿用 `ProgressDialog` 的任务生命周期思路。总体任务进度不一定是字节数，速度与 ETA 只对字节下载阶段计算；解压／解析不能伪装成下载速度。
- **可信包**：已有 `InstanceImportTask` 的 `trustedSource` 参数，传入 Modrinth 创建任务后可跳过该来源警告（`ModrinthInstanceCreationTask.cpp:462`）。若启用既有 PDF 中的自家包豁免，应只在固定、识别过的新岛入口使用，普通导入仍按原逻辑。

### 必须处理的两个细节

1. `performMainStartupAction` 后续还有启动器更新器初始化与 `m_mainWindow->processURLs`；不能只把默认主页换掉而留下空指针。先保持原 MainWindow 所需生命周期，或显式让外部导入路径创建它，并核查更新器父窗口。`Application::launch` 的账号弹窗父窗口也要在只有开屏时正确关联。
2. `InstanceTask::downloadFiles` 当前在游戏文件预下载失败后提示警告，仍调用 `emitSucceeded()`。因此“实例导入完成”和“运行文件准备完毕”不是同一个状态。建议保留原导入语义，并增加可查询的预下载结果／信号供开屏显示“实例已安装，运行文件待补齐”；不要只凭最终 succeeded 显示“全部下载完成”。需要时提供重试或允许启动流程继续补齐。

## 6. 建议的落地顺序与验证

1. 用户提供镜像根地址与整合包地址后，先做配置方式的最小验证：目标版本完整 meta 依赖闭包、资源索引、主 JAR、库、Forge 文件和资源对象。
2. 检查冷缓存下请求实际命中镜像、没有被重定向回官方；再检查热缓存可复用，故意损坏的文件被原校验拒绝。核对目录及 URL 转义，不关闭校验。
3. 确定是否需要第 4 节的客户端补丁；没有证据时不先写网络重定向代码。
4. 实现开屏及原任务接线，先做下载、启动和进入 Prism 三个入口；额外更新系统不纳入本轮。
5. 验证无实例／已有实例／多个实例、导入失败、游戏文件预下载失败、取消与关闭、无账号及外部 URL 导入。此前文件链接辅助程序弹窗问题在运行相关测试前先解决。

以上为实施前静态分析。随后采用现有配置方案并实现开屏；详见 [开屏实现与验证](开屏实现与验证.md)。实际镜像地址未确定，目标版本的真实镜像下载链路仍需验证。
