# copyparty 镜像准备

## 2026-10-30 状态更新：正式下载地址统一到 ashbill

用户要求：启动器默认入口与服务器镜像元数据中的下载地址统一使用 ashbill 域名。以后生成、更新或重新部署镜像时，也必须按以下根地址定稿，不能重新上传带 yake 绝对地址的旧元数据。

- 发布根：`https://mc1.ashbill.com:25572/synology-root/minecraft/niijima-launcher/`。
- 镜像根：发布根加 `niijima-mirror/`；元数据入口加 `meta/v1/`，资源入口加 `assets/`。
- `modrinth.index.json` 是整合包的 Mod 清单；本节的 7 个 JSON 是 Prism 的 Minecraft／Forge／LWJGL 基础资源元数据，两套清单不能混淆。只改启动器入口不会自动改写元数据里的绝对下载 URL。
- 已获用户批准，通过自研共享终端在 `custom-sz-cn` 更新 `/synology/MCShare/minecraft/niijima-launcher/niijima-mirror/meta/v1/`：3 个版本 JSON 共 174 处 URL 改址，并重算 3 个组件索引及根索引的 SHA-256，共替换 7 个文件。
- 执行前核对全部旧文件哈希，完整备份至 `/home/ashbill/niijima-mirror-meta-backup-20261030/`；执行后核对全部新文件哈希。备份保留原地址，属于回退资料，不参与线上下载。
- 北京时间 2026-10-30 01:39，实际 HTTP 获取 7 个 JSON 均成功且匹配预期哈希；版本→组件→根索引链有效，174 个下载 URL 全部指向上述 ashbill 镜像根，无 yake 残留。
- 使用既有 nginx `/synology-root` 静态映射，未修改 nginx、重启服务或改动游戏资源。没有扫描或替换其他无关服务的域名配置。正式 `packs/latest.json` 上传状态另见整合包版本索引记录；本次不代表完成真实游戏安装。

**历史范围说明：** 下方 2026-10-29 归档中的 yake 地址已过时；单归档部署方式和游戏版本范围仍可参考，但旧归档不能原样覆盖本次元数据。后续发布应使用新根地址并重新核对元数据哈希链。

## 2026-10-29 最终交付：一个包上传后原地解压（唯一当前方案）

用户明确要求：只上传一个 `.tar.gz` 到现有 copyparty 的固定父目录，再在原地解压；**不增加目录搬动、元数据补丁、服务器脚本或软件安装步骤**。下文早期 `publish/` 根、补丁和服务器定稿流程仅保留历史，均不再用于本次部署。

- **唯一交付文件**：`D:/mydata/Agents/Minecraft相关/本地资源/Prism镜像部署/上传即用-20261029/niijima-mirror.tar.gz`，**673,669,096 字节（642.46 MiB）**。
- 上传位置：`https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/`。上传后就在此目录解压。归档自带顶层 `niijima-mirror/`，其内直接是 `assets/`、`files/`、`meta/`，不含 `publish/` 包装、脚本、清单、README 或空的 `packs/`。
- 最终镜像根固定为 `https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/niijima-mirror/`，所有元数据已在本地按此 URL 离线定稿。**没有 `/publish/`**。
- `MetaURLOverride=https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/niijima-mirror/meta/v1/`
- `ResourceURLOverride=https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/niijima-mirror/assets/`
- 整合包发布与镜像分开：用户已确认在同级 `packs/latest.json` 保存多个数字 ID 记录，默认下载最大 ID 的文件；首项为 `新岛铁道 v1.6.zip`，见《整合包版本索引》。相应源码已构建，版本索引测试通过；线上 JSON/ZIP 仍需维护者上传。本镜像归档不含 ZIP 或发布索引，无需因这次功能修改重新打镜像包。Java 下载和账号服务仍沿用原流程。
- 交付目录另有包外极简 `部署说明.md`，留本地供用户阅读，不需上传。此前 `niijima-mirror-mc1.20.1-forge47.4.6-20261029.tar.gz`、`仅部署文件-20261029/` 内的完整包和补丁均已被本次单包替代，未来 agent 不再要求用户使用它们。

启动器默认值与本包已对齐并完成构建：仅 Niijima 品牌在设置键缺失时采用上述两个镜像；用户已有自定义值或显式空值保留，显式留空继续使用官方源。本机现有预览配置没有这两个键，也会直接采用新默认。不改客户端下载代码，不新增 nginx 规则，不修改用户配置/缓存。EXE 和安装器位置及测试结果见《Windows安装器打包》末节。正式整合包直链仍独立配置。

范围保持 Minecraft **1.20.1**、Forge **47.4.6**、LWJGL **3.3.1**。复用此前按官方 SHA1/字节数验证的 **3,746 个资源文件，767,710,792 字节**；本次复制前核对清单和各源文件字节数，资源闭包不变，未重新下载。重新生成的 **7 个 JSON** 已检查版本→组件→根索引 SHA256 链，**174 个下载 URL** 全部指向最终无 `publish/` 根地址。归档共 **3,753 文件**；已用 PATH 上的 7z 完成 tar 和 gzip，两步均退出 0、输出 `Everything is Ok`，按用户规则未额外测试或解包压缩包。

本地构建和验证记录：`D:/mydata/Agents/Minecraft相关/本地资源/Prism镜像部署/构建暂存-20261029-final/verification.json`。本节只确认本地最终归档已完成；未上传、未删除远端旧文件，尚未验证新无 `publish/` URL 上线或真实游戏安装。旧 `publish/` 部署的访问结果不能作为新地址验收；用户完成本次原地解压后，再按新根做只读检查。

## 2026-10-29 全量整合包与安装下载量评估（未构建）

只读核查结果；没有下载 JAR、运行 build-all、安装启动器或上传远端。

- 已有 Windows 安装器：仓库外 本地资源/启动器安装器/2026-10-29/NiijimaLauncher-Setup.exe，31,902,037 字节，2026-10-29 16:52 版；可沿用它做安装流程测试，不需仅为全量包重编启动器。
- 打包入口属于整合包仓库：build-all.bat → scripts/build.ps1 -Mode all。现有 test 分支已实现 Expand-Index(..., true)，会下载全部索引条目到临时 overrides 并清空临时索引 files；但还会改音乐、FOV、灵敏度和世界昼夜/天气，不能直接作为正式全量包。
- 建议新增正式 full 类型，复用 Expand-Index 的全部展开与缓存逻辑，不调用 Set-TestOptions/Set-TestWorldRules，保留 dependencies 和其余正式配置。原始 pack/modrinth.index.json 不变，只修改临时发行目录。Niijima 仍走原生 Modrinth 导入，无需改变游戏下载代码；files 为空可避免再次请求那些已内置资源。
- 当前开发分支 agent-dev-1.6：111 条下载，496,831,660 字节，其中 mods 109 条 / 376,247,262 字节，AshBill 1 条 / 120,435,924 字节，shaderpacks 1 条 / 148,474 字节。overrides 1,521 文件 / 139,917,939 字节，与索引目标路径无重叠。build/cache 中没有对应大小匹配的索引文件；这是大小盘点，不是哈希验证，也不代表其他游戏实例里没有缓存。
- 现有 build/pack.zip 为 2026-10-25 产物，130,908,027 字节，仅可作为压缩体积参考。开发分支正式全量 ZIP 预估约 600–640 MB（十进制），准确数字待实际打包。源文件总量约 636.75 MB，JAR/MP4 等再次压缩收益有限。
- 注意 build-all 的普通正式版来自本地 master，而非当前工作分支。master 索引是 1.6.0、98 条、256,758,036 字节，与上述当前开发包不同；正式制作前需明确发行源，不能混用上述大小估计，也不能不说明就发布旧分支。
- MC 1.20.1 / Forge 47.4.6 / LWJGL 3.3.1 镜像清单全平台 3,746 文件 / 767,710,792 字节。按现有组件库 rules 的 Windows 平台筛选，预计 3,699 文件 / 756,565,127 字节；元数据未出现额外 arch/version/features 规则或 native classifier 映射。此为本地清单静态估算，不是实际安装网络流量实测。
- Windows 明细：资源对象 651,642,397 字节，Minecraft 客户端 23,028,853 字节，Minecraft/LWJGL 库 60,206,330 字节，Forge Maven 文件 21,243,792 字节，ForgeWrapper 29,800 字节，资源索引 413,067 字节，日志配置 888 字节。元数据和 HTTP 开销另有少量增量。
- 部署用 tar.gz 为 673,669,096 字节，但客户端逐文件获取，不下载这个 tar.gz，因此不能把部署压缩包大小当作玩家依赖下载量。
- 当前开发全量包 + Windows 游戏依赖 + 安装器，预计约 1.39–1.43 GB（十进制），不含 Java；没有可复用 Java 时还会单独下载，大小取决于原生自动选择的运行时。已有资源缓存会减少实际下载量。
- 补充源码发现：AutoInstallJava.cpp 通过 metadataIndex 的 net.minecraft.java 列表选择 Java；Meta/BaseEntity.cpp::url() 对该列表同样应用 MetaURLOverride。当前准备的镜像根索引只有三个游戏组件，没有 net.minecraft.java；在没有可复用运行时或明确 Java 路径的干净环境下，自动 Java 元数据请求无法由该镜像满足。测试前需要补入相应 Java 列表/版本元数据及其运行时来源。此前“Java 沿用原流程”描述不充分，不能理解为它会自动绕过自定义元数据镜像。此问题为本次源码核对所得，尚未修复或实际安装复现。

- 推荐测试流程：维护者上传正式 full ZIP 到 packs/，更新已约定的 latest.json，再使用独立数据目录测试安装器 → 下载整合包和游戏依赖 → 启动；不擅自上传、创建发布或打开窗口。本节是讨论方案，不代表已执行或增加待办。


## 2026-10-29 实际 copyparty 地址与首次交付修正（历史方案，已被上节替代）

用户已将旧归档整个解包目录上传，当前实际镜像根是其下的 `publish/`：

`https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/niijima-mirror/publish/`

只读访问确认根目录中同时公开了 `_sources/`、脚本和清单；线上元数据仍含 `mirror.invalid`。旧包虽不含 copyparty 软件，但让用户处理发布目录映射、运行本地可完成的定稿脚本，不符合直接上传已有 copyparty 的交付需求。以下为最新状态，后文“尚未上传/URL 未知”均是旧交付时的记录。

- 本地已按真实 URL 离线定稿并通过 7 个 JSON、版本→组件→根索引 SHA256 链及 174 个下载 URL 核查。原资源清单不变；没有重新下载大文件。
- 新交付目录：`D:/mydata/Agents/Minecraft相关/本地资源/Prism镜像部署/仅部署文件-20261029`。`部署说明.md` 与 `启动器镜像配置.txt` 放在归档外。
- 现有部署仅需 `niijima-mirror-meta-patch-20261029.tar.gz`（12,627 字节），解压得到 `meta/`，合并覆盖到线上 `niijima-mirror/publish/`，共 7 个 JSON。资源无需移动或重传；补丁尚未由用户上传。
- 完整替代归档 `niijima-mirror-static-mc1.20.1-forge47.4.6-20261029.tar.gz`（673,646,896 字节），根目录只有 `assets/`、`files/`、`meta/`。解压到同一个 `publish/`。不含脚本、审计清单、原始 JSON 缓存或额外包装目录。
- 旧包多余公开的辅助文件共 11 个、1,837,969 字节；运行资源为 3,575 个 assets、171 个 files、7 个 meta。几千个资源对象是指定游戏版本所需文件，不能直接删掉。
- 远端只做目录/元数据读取和两个小文件的 HTTP 200 + SHA1 抽查；未全量远端校验、未上传/删除、未完成真实安装或进游戏。旧部署状态 JSON 不能代替线上元数据实际状态；不要把本地补丁生成说成线上已修复。
- `packs/` 为空，正式整合包 URL 仍需另给；不把不存在的 pack.zip 填成可用地址。`MetaURLOverride` 为上述根加 `meta/v1/`，`ResourceURLOverride` 为根加 `assets/`。
- 以后本任务交付采用纯静态归档：脚本、审计资料和说明留本地/归档外；URL 不确定时先交资源，收到实际 URL 后在本地定稿并交小补丁，不要求用户为了这个镜像安装软件或在服务器运行生成器。

本轮没有构建启动器或修改其配置；此前 UI 的 DPI 修复仍待明确构建。归档按 7z 成功退出验收，没有追加解包测试。

更新：2026-10-29。范围：本地整合包当前使用的 **Minecraft 1.20.1 / Forge 47.4.6**，以及组件依赖 **LWJGL 3.3.1**。采用 Prism 原有配置和服务端元数据改写，不修改启动器下载路由。

## 已准备的文件

本机目录：`D:\mydata\Agents\Minecraft相关\本地资源\Prism镜像准备`。

- `download-manifest.json`：每个上游文件的官方直链、镜像相对路径、SHA1、字节数。
- `official-sources.txt`：完整上游 URL 列表。
- `publish/`：已生成元数据和原始资源索引；**3,746 个资源文件已全部取得并按官方 SHA1/大小校验；尚未上传**。
- `_sources/`：构建元数据使用的原始 JSON 缓存，不上传。

本次去重后共 **3,746 个上游文件，767,710,792 字节（约 732.15 MiB / 0.715 GiB）**。包含组件元数据列出的跨平台原生库，不只是 Windows；不包含整合包 ZIP、Java、模组下载、元数据自身或服务器 JAR。

实际镜像 URL 未确定，现在使用保留域名 `https://mirror.invalid/niijima/` 生成示例。资源包可以先解压部署，但启动器使用前必须按最终 URL 完成元数据定稿；现提供完全离线的定稿脚本，自动重算所有元数据摘要。

## 发布目录和 URL

假设 copyparty 将 `publish/` 映射为 `https://你的域名/niijima/`：

```text
publish/
├── packs/
│   └── pack.zip                         # 手工放入审核后的新岛整合包
├── meta/v1/
│   ├── index.json
│   ├── net.minecraft/
│   │   ├── index.json
│   │   └── 1.20.1.json
│   ├── net.minecraftforge/
│   │   ├── index.json
│   │   └── 47.4.6.json
│   └── org.lwjgl3/
│       ├── index.json
│       └── 3.3.1.json
├── files/
│   ├── piston-data.mojang.com/...       # Minecraft 客户端
│   ├── piston-meta.mojang.com/...       # 原样保留的资源索引
│   ├── libraries.minecraft.net/...      # Minecraft / LWJGL 库
│   ├── maven.minecraftforge.net/...     # Forge installer 及依赖
│   └── files.prismlauncher.org/...      # ForgeWrapper 等 Prism 依赖
└── assets/
    └── <hash前两位>/<完整hash>           # 声音、材质等资源对象
```

让客户端直接 GET 对应文件，而不是目录浏览页面。下载地址需可由玩家访问；不要将需要交互登录的页面 URL 当成直链。完整 host 列表和路径以清单为准。

以下表格是早期手工配置方案的示例。当前源码已按用户要求删除自动配置弹窗，采用《整合包版本索引》的固定索引 URL 和可选配置覆盖；清空 NiijimaPackUrl 走索引，不再弹窗。该改动已随本轮明确授权构建生效。

| 输入框 | 配置键 | 示例值 |
| --- | --- | --- |
| 整合包地址 | `NiijimaPackUrl` | `https://你的域名/niijima/packs/pack.zip` |
| 元数据镜像 | `MetaURLOverride` | `https://你的域名/niijima/meta/v1/` |
| 资源镜像 | `ResourceURLOverride` | `https://你的域名/niijima/assets/` |

镜像留空使用 Prism 原来源。**两个镜像设置全局生效**，包括从「进入 Prism」管理的其他实例；此最小元数据服务只列出以上版本。要安装其他版本，先清空镜像设置恢复原来源，或另行扩展镜像清单。更换镜像后，已有组件元数据缓存可能仍含旧 URL；冷缓存测试使用新的独立数据目录，不直接删除用户数据。

## 生成与下载命令

脚本：`tools/niijima/prepare_mirror.py`，Python 3.10+，PATH 上的 curl。默认只下载小体积 JSON 并生成清单，不下载二进制文件。

```powershell
python -X utf8 'D:\mydata\Repositories\PrismLauncher\tools\niijima\prepare_mirror.py' `
  --output 'D:\mydata\Agents\Minecraft相关\本地资源\Prism镜像准备' `
  --base-url 'https://你的真实域名/niijima/'
```

准备完整发布树时，在同一命令末尾添加 `--download-files`；如需显式代理，再添加 `--proxy http://127.0.0.1:44554`。脚本串行下载、显示 curl 速度/进度、按上游 SHA1 和字节数检查、失败即停止；已完成且摘要匹配的文件可复用。单文件超时 30 分钟。没有上传、部署或删除服务端文件的逻辑。

成功下载全部清单文件后才生成 `download-complete.json`，清单生成不能替代这个完成标志；整合包 ZIP 仍需自行加入。更换 Minecraft/Forge 范围时使用新的输出目录，避免旧版本文件残留；本次 `_sources` 缓存固定当前上游快照，想更新上游元数据也使用新的输出目录。

不要只改版本 JSON 内的 URL：脚本同时重算组件索引、根索引中的 SHA256。资源索引内容保持原样，保留原 SHA1；资源对象通过 `ResourceURLOverride` 改变地址。

## 官方来源

| 用途 | 来源 |
| --- | --- |
| Prism Minecraft 元数据 | [net.minecraft/1.20.1.json](https://meta.prismlauncher.org/v1/net.minecraft/1.20.1.json) |
| Prism Forge 元数据 | [net.minecraftforge/47.4.6.json](https://meta.prismlauncher.org/v1/net.minecraftforge/47.4.6.json) |
| Prism LWJGL 元数据 | [org.lwjgl3/3.3.1.json](https://meta.prismlauncher.org/v1/org.lwjgl3/3.3.1.json) |
| Mojang 1.20.1 客户端 | [client.jar](https://piston-data.mojang.com/v1/objects/0c3ec587af28e5a785c0b4a7b8a30f9a8f78f838/client.jar) |
| Mojang 资源索引 | [1.20.1 对应 asset index 5](https://piston-meta.mojang.com/v1/packages/62b36534d83c028050e6fafe8c742879d1f3f1ca/5.json) |
| Forge 发布页 | [Minecraft 1.20.1](https://files.minecraftforge.net/net/minecraftforge/forge/index_1.20.1.html)（仍固定 47.4.6，不取最新） |
| Forge installer | [forge-1.20.1-47.4.6-installer.jar](https://maven.minecraftforge.net/net/minecraftforge/forge/1.20.1-47.4.6/forge-1.20.1-47.4.6-installer.jar) |
| ForgeWrapper | [prism-2026-08-01.jar](https://files.prismlauncher.org/maven/io/github/zekerzhayard/ForgeWrapper/prism-2026-08-01/ForgeWrapper-prism-2026-08-01.jar) |

Minecraft 客户端和全部依赖的精确直链都在 `download-manifest.json` / `official-sources.txt`，避免手抄几千条路径。Forge installer 本次 SHA1 `c256d27349c6ce4b3a527fa61a89498997251ebb`，大小 6,077,386 字节。

## 覆盖边界与上线验收

已分析的是 Prism 元数据所声明的文件与资源对象。Java 自动下载、Microsoft 登录、Modrinth/CurseForge API、整合包索引中的模组直链和启动器自身更新不在此镜像范围内。Forge 首次启动还包含安装器处理步骤；真实服务器尚未配置，不能把静态闭包当成整套游戏已离线验证。

URL 确定并上传后，用独立空数据目录验证：导入真实 pack.zip、查看 copyparty 请求和 Prism 日志、完成 Java/Forge 首次处理、登录并进主菜单；再重启验证缓存复用。镜像中断时沿用原任务失败/重试提示，本轮没有添加自动回退官方源；用户可以清空镜像配置恢复来源。首次启动要保留原文件完整性校验。

## 2026-10-29 完整部署 tar.gz

交付目录：`D:/mydata/Agents/Minecraft相关/本地资源/Prism镜像部署`。

- 压缩包：`niijima-mirror-mc1.20.1-forge47.4.6-20261029.tar.gz`。
- 解包目录：`niijima-mirror/`，copyparty **只公开其中的 `publish/`**。
- 部署说明：包内 `README.md`；独立副本为交付目录的 `README-deploy.md`。
- 3,746 个上游资源，共 767,710,792 字节，逐文件 SHA1/长度一致；包含本机既有 Minecraft 缓存的校验复用和官方源补缺下载。未修改缓存源文件。
- 不包含正式 `pack.zip`、Java、账号服务或模组下载镜像。`publish/packs/` 留空，整合包应另外提供或使用独立 URL。
- 最终网站根 URL 尚未提供，因此元数据仍有 `mirror.invalid` 占位，`deployment-status.json` 明确为 payload 完整 / metadata 未定稿；不能把包已生成说成启动器已能使用镜像。

服务器有 Python 3.10+ 时，解压后执行：

```sh
tar -xzf niijima-mirror-mc1.20.1-forge47.4.6-20261029.tar.gz -C /srv/copyparty
python3 /srv/copyparty/niijima-mirror/configure_mirror.py \
  --base-url 'https://你的域名/niijima/'
```

将 `/srv/copyparty/niijima-mirror/publish/` 映射到该 HTTPS 根 URL。`configure_mirror.py` 和 `prepare_mirror.py` 位于解包根目录，使用 `_sources/` 的冻结上游 JSON，完全不联网；重写绝对 URL、重算版本→组件→根索引的 SHA256，保持资源文件列表、SHA1 和字节大小不变。生成 `launcher-settings.txt` 和更新后的 `deployment-status.json`。不要只用文本替换 URL。

也可先部署文件，再把 URL 提供给 agent：先检查资源可达性，再在本机给出仅含 `publish/meta/v1/` 的小补丁，覆盖服务器元数据；不需要再次上传整个资源包。资源和元数据分别完成，不改客户端下载代码。

### 下载与验证记录

原 curl+44554 可下载部分 Mojang 小资源，但对后续 Forge/libraries 请求连接超时；10809 当时不可用。改用 Windows/.NET 系统线路后补齐大部分，最后三个 EOF/TLS 中断文件通过 curl `--noproxy '*'` 直连取得，均匹配原清单摘要。没有关闭 TLS/哈希验证，也没有使用第三方源替换；这只是 2026-10-29 的线路表现，不是永久代理排名。

可复用的 Windows 下载入口为 `tools/niijima/download_mirror_windows.ps1`（PowerShell 7+，最多四路，错误停止后保留已验证文件），例如：

```powershell
pwsh -File tools/niijima/download_mirror_windows.ps1 -Output 'D:/mydata/Agents/Minecraft相关/本地资源/Prism镜像准备'
```

该脚本读取已生成清单，复用已校验缓存，只有全部文件匹配才写 `download-complete.json`，不要求下载时已经知道最终网站 URL。串行 curl 下载仍可用旧生成器的 `--download-files`。

离线定稿已在隔离临时目录连续验证两个不同根 URL：所有下载字段前缀、版本/组件/根索引摘要链、3,746 文件闭包保持一致，无完整下载凭据时不伪造完成标志。PowerShell 下载器通过语法检查，缓存全命中路径实测通过；本批实际补缺使用同样的 Invoke-WebRequest + SHA1/大小核对逻辑。

归档使用 PATH 上 7z 先 tar 后 gzip，两步退出码均为 0、输出 Everything is Ok；按照用户约定没有额外解包/归档完整性测试。尚未上传、部署或实际通过此镜像启动游戏。

压缩包大小：676,415,492 字节（645.08 MiB）。

### 2026-10-29 用户确认的范围

用户接受保留其他平台，以便未来使用；要求不夹带其他 Minecraft 版本。已核对组件仅 `net.minecraft 1.20.1`、`net.minecraftforge 47.4.6`、`org.lwjgl3 3.3.1`。3,746 文件中有 3,575 个资源对象（651,642,397 字节）和 171 个本体/索引/库文件（116,068,395 字节）。明确 Linux/macOS 文件 37 个、5,291,298 字节，另有 Windows 不同架构；这是指定版本的完整镜像，不是 Windows x64 最小集。声音、语言等资源本来按索引拆分为大量小文件，数量不是模组数。
