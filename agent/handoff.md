# PrismLauncher 交接

## 当前迭代约定（2026-10-29 用户明确要求）

当前版本已按用户明确要求构建：顶栏/标题组/按钮微调、统一blur18无遮罩、内置 Zen Maru Gothic Regular、移除地址弹窗、packs/latest.json 数字 ID 版本选择，均已进入 EXE/安装器。CTest 23/23 与独立 offscreen 视觉检查通过；实际索引部署和游戏安装仍待验证，详见末节。后续修改仍需用户明确要求才构建。

产品交互约束：开发时缺信息先与用户确认方案，不得把问题转成未经用户要求的产品配置窗口或新流程；保留配置文件覆盖即可。用户已确认多版本 JSON 以数字字符串 ID 为键，默认最大 ID，可配置指定 ID。

当前唯一镜像交付为仓库外 `本地资源/Prism镜像部署/上传即用-20261029/niijima-mirror.tar.gz`。用户只需上传到既有 `minecraft/niijima-launcher/` 后原地解压；包内自带 `niijima-mirror/`，没有 `publish/`。对应镜像 URL 已作为 Niijima 品牌默认值内置并构建，等待用户部署；旧包及补丁方案不再使用。详见《copyparty镜像准备》顶部。

此前构建完成：修复字标 DPI 裁切、34 px/2 px 字距和加宽按钮、内置最终镜像默认值，均已进入新的测试 EXE 和安装器；具体尺寸及验证结果见末节。后续修改仍需用户明确指示才构建。

- 修改后先保留源码/文档，**只有用户明确指示构建时才配置、编译、收集运行目录或重打安装器**；未构建的回复应明确写“未构建”。不要因代码修改自动触发 CMake/Ninja/NSIS。
- 用户明确构建后，完成适用验证，再按功能/依赖关系写一个或多个本地 commit，并提醒用户将对应源码 push 到公开仓库。用户已授权这次后续构建完成后的分类提交；展示分组 diff 与验证结果，不重复索取相同授权。未相关的其他工作另行判断。
- Agent 不执行 push，不创建远端发布。源码 URL 为 `https://github.com/ashbillzw/PrismLauncher`；commit 不等于 push，源链接存在不等于云端已含本地修改。
- 当前可直接运行的成品为 `install-niijima/niijima-launcher.exe`（2026-10-29 Regular 外观与数字版本索引版），与 DLL、Qt 插件等配套使用。本轮截图、署名、文件名和图标分离已包含；暂不自动显示启动器或安装器窗口。


更新：2026-10-29。仓库 `D:/mydata/Repositories/PrismLauncher`，当前分支 `develop`。本轮构建前基线为 `78a1aa051`，实施时与本地缓存的 `origin/develop` 对齐；本轮 Regular 资源及首页/版本索引在构建完成后分组提交。未 fetch 或 push，最新工作区和 ahead 状态以本地 Git 查询为准；后文未提交/未构建等为对应历史迭代记录，以本节及末节为准。

## 当前工作与用户决定

制作新岛铁道专用开屏，复用原整合包安装和启动路径。镜像用现有 `MetaURLOverride` / `ResourceURLOverride` 加改写后的服务端元数据，**不修改游戏下载代码**。镜像 URL 已固定并内置；包目录采用已确认的 packs/latest.json 数字 ID 索引（已构建）；背景使用用户确认的确定版图片，统一轻微模糊且不加遮罩（2026-10-29）；允许自动下载/切换 Java，账号登录留到首次启动游戏。

**目前不要启动会显示窗口的程序。用户要求等其回到电脑前再继续可视化验证。** 本次测试进程已退出。不要把 Start-Process 的 Hidden 当成保证 GUI 不会显示的机制。

## 入口

- [启动器初步计划](启动器初步计划.md)：最新需求及原 PDF 视觉约定。
- [开屏实现与验证](开屏实现与验证.md)：代码接线、配置、测试记录、未完成验收。
- [整合包版本索引](整合包版本索引.md)：packs 目录、数字 ID 选择规则、发布 JSON 与配置项。
- [copyparty镜像准备](copyparty镜像准备.md)：目录、完整清单位置、官方源、下载脚本。
- [Windows安装器打包](Windows安装器打包.md)：当前 NiijimaLauncher 安装器、系统隔离与确切编译命令。
- [本机构建记录](本机构建记录.md)：Qt/VS/JDK 路径及历史辅助程序弹窗问题。

大资源、日志和测试数据仍在仓库外 `D:/mydata/Agents/Minecraft相关`。元数据与清单：`本地资源/Prism镜像准备`；镜像下载已完成，见下方 2026-10-29 交付记录。字体和约 253 KiB 中文翻译因需要随程序发布而放入源码资源目录。

工作区起初已有《启动器初步计划.md》修改及未跟踪《Prism开屏与镜像源码核查方案.md》，本次在其上继续实现。后续先检查 git status；2026-10-29 用户已授权明确构建完成后的分类 commit，无需重复确认相同授权。不 push，不触碰整合包仓库原有工作。

## 本轮检查

- 已成功首轮构建并收集 `install/prismlauncher.exe`。
- 22 项现有测试通过（排除 FileSystem）；镜像脚本 3 项测试通过；完整镜像元数据摘要链检查通过。
- GUI 交互、真实包导入、真实镜像请求和游戏启动尚未验证；用户回来后继续。

最终增量构建状态在本文件末尾记录。运行界面前使用独立 `--dir`，不要写入用户正式 Prism 配置。也不要运行 FileSystem CTest，除非先解决 filelink 辅助程序 DLL 和提权环境问题，避免再次出现重复模态弹窗。

最终结果：最新增量构建及 install 收集成功（退出码 0）；随后 CTest 22/22、镜像脚本 3/3 通过，git diff --check 通过。未再次启动 GUI。所有修改保留在工作区，未提交。

## 2026-10-28 背景配色预览（历史：次日已获准应用）

用户提供原图 `C:\Users\ashbill\Desktop\46F7320C81F94FEF406BB022561E9F64.png`，先要求 imagegen 调色预览，后选中第一版较亮、较鲜艳的结果，要求改用传统算法从原图尽量复刻，并稍微提亮增艳。确定版已完成；仅固定 RGB 色彩映射、选择性色彩、曝光及色度调整，没有生成式重绘或空间像素混合，保持原图 2559×1439 和 alpha。已核对输出尺寸、alpha 及 5000 个抽样像素的同位置色彩函数结果。

- 桌面成品：`C:\Users\ashbill\Desktop\新岛铁道背景-确定版.png`。
- 桌面对比：`C:\Users\ashbill\Desktop\新岛铁道背景-三版对比.png`，从上到下为原图、第一版 imagegen、确定版。
- 参考 imagegen：`C:\Users\ashbill\.codex\generated_images\01a109d4-c497-7271-a2d8-74619cfaad26\exec-40097b45-2102-4956-b7fd-a26fd0f58e55.png`。
- 脚本、固定参数、迭代试片和处理记录：`D:\mydata\Agents\Minecraft相关\本地资源\启动器背景调色\2026-10-28`，入口 `README.md` / `final_grade.py` / `deliver.py`。

上述为 2026-10-28 的预览交付记录；当日尚未获准应用。2026-10-29 用户已确认应用，旧的橙色渐变占位由确定版背景替换，见下节。

## 2026-10-29 应用背景与磨砂玻璃

- 确定版已原样复制到 `launcher/resources/niijima/home-background.png` 并列入 QRC，不依赖用户桌面路径；资源来源记录在同目录 `README.md`。
- 开屏对图片预先进行轻度模糊并缓存，等比居中铺满主体，叠加半透明玻璃及对比度渐变；文字和按钮不模糊，橙色标题栏和主按钮保留。代码及参数见 [开屏实现与验证](开屏实现与验证.md)。
- 用户之前要求暂不显示启动器窗口仍然有效。本轮不运行启动器，也不执行会反复弹窗的 FileSystem 测试；构建结果另列下方。GUI 外观、高 DPI 和交互需后续由用户允许后验收。

本轮结果（2026-10-29）：Release 增量编译及 `cmake --install` 均成功（退出码 0），产物 `install/prismlauncher.exe` 已更新；`git diff --check` 通过，内置背景与桌面确定版 SHA-256 相同。未启动 GUI，未重跑与本次视觉改动无关的功能测试。所有修改仍未提交、未推送。

## 2026-10-29 截图反馈与字体规范

用户标注要求已落实：左上标题白色、保留英文标识；删除额外大标题/宣传语及首页设置/选择已有实例按钮；进入 Prism 按钮改为较小的棕灰色样式。任务状态按需显示，原下载/启动、取消、既有绑定和进入 Prism 接线保留。包 URL 缺失或非法时点击下载可进入配置，更换有效 URL 的方式见镜像文档。

日文文字（包含 kanji）默认 Zen Maru Gothic，中文默认 Noto Sans SC；后者已连同 OFL 内置，前者继续使用现有内置字体。开屏中文控件和子对话框使用 Noto Sans SC，日文控件显式标记语言；原 Prism 主窗口不受这份开屏样式影响。详见 [开屏实现与验证](开屏实现与验证.md)。本轮仍不主动打开启动器窗口，不 commit 或 push。

## 2026-10-29 后续微调

用户确认追问的两套字体均已内置；新要求为主按钮字稍大且白色、进入 Prism 后隐藏开屏、磨砂稍弱。已改为 20 px 白字、背景模糊半径 20 / 浅色雾层 alpha 20。进入 Prism 会隐藏开屏；关闭 Prism 时，空闲开屏随之关闭，有进行中的导入则恢复开屏供继续查看/取消。实现和生命周期说明见 [开屏实现与验证](开屏实现与验证.md)。构建及实际 GUI 验收状态应分别记录，不主动显示启动器。

本次微调验证：Release 编译和 install 收集成功，git diff --check 通过；主窗口/隐藏开屏的关闭计数经源码审查。程序仍在 install/prismlauncher.exe。本轮未启动 GUI，字体显示及切换/退出交互尚未实测。构建日志位于仓库外工具/prism-build/home-polish-build.log，未提交。

## 2026-10-29 镜像部署包与 AshBill 安装器交付

- 用户确认可保留其他平台文件，要求仅覆盖当前 Minecraft/Forge 版本；三组件固定为 MC1.20.1 / Forge47.4.6 / LWJGL3.3.1，没有其他 Minecraft 版本。
- 镜像交付：`D:/mydata/Agents/Minecraft相关/本地资源/Prism镜像部署/niijima-mirror-mc1.20.1-forge47.4.6-20261029.tar.gz`，676,415,492 字节。3,746 个文件全部官方 SHA1/长度匹配（含只读复用 PCL2 缓存）；7z tar/gzip 成功，无额外解包测试。不含整合包 ZIP、Java 或模组镜像。
- 只公开解包后的 `niijima-mirror/publish`。最终 URL 未定，metadata 仍为占位；用户部署后给 URL，再检查并生成小份元数据补丁。也可运行包内离线 configure_mirror.py 定稿 URL 并重算摘要链。不要宣称已上线或真实游戏已通过镜像运行。
- 独立 AshBill 版本已完成 Release 全量构建和 NSIS 编译：`install-ashbill/ashbill-niijima-launcher.exe`、`build-ashbill/dist/AshBillNiijimaLauncher-Setup.exe`。最终交付副本在仓库外 `本地资源/启动器安装器/2026-10-29`。构建方法、系统隔离和验证边界见《Windows安装器打包》。默认安装在 LocalAppData/Programs/AshBill/NiijimaLauncher，数据默认 AppData/AshBillNiijimaLauncher；仅独立 AshBill 协议与卸载键，不抢普通 Prism 关联，不使用官方自动更新。
- 主按钮 24 px 白字；中心内容未添加。特殊标题已核对 Zen Maru 不含「鉃」U+9243，按用户回复保留原字，SVG 候选仅在仓库外供讨论。两套字体仍内置。
- 未运行安装器、未显示启动器窗口、未修改本机注册表、未签名、未 commit/push。后续允许可视化验证时再测真实安装/卸载、GUI、账号登录、整合包安装及游戏启动。

最终补充：独立 AshBill 构建 CTest 22/22 通过，排除 FileSystem 弹窗测试；UTF-8 NSIS 编译成功，安装器 30,437,181 字节已复制到仓库外。git diff --check 通过。仍未运行安装器/启动器，未签名、未 commit/push。


## 2026-10-29 更正产品名为 NiijimaLauncher

用户澄清 AshBill 前缀仅为避免与原 Prism 冲突，不是品牌要求。产品改为 NiijimaLauncher；EXE/cfg/URL scheme 为全小写 niijimalauncher；默认安装 `%LOCALAPPDATA%/Programs/NiijimaLauncher`，数据 `%APPDATA%/NiijimaLauncher`，安装键 `HKCU/Software/NiijimaLauncher`，卸载键末段 NiijimaLauncher，开始菜单 NiijimaLauncher。开发者域名、Publisher 及 `com.ashbill.NiijimaLauncher` AppID 保留。此前 AshBill 前缀交付记录仅作历史。

当前用 `niijima` preset 复用 `build-ashbill` 缓存，收集到全新 `install-niijima`；NSIS 模板改为 `win_install_niijima.nsi.in`。交付名称 `NiijimaLauncher-Setup.exe`。详见安装器专题；仍不打开 GUI、运行安装器、commit 或 push。


## 2026-10-29 自有标志

用户指定群图标 https://p.qlogo.cn/gh/549428882/549428882/0 作为 EXE 等处 logo 的基础。橙白 N 标志已整理为 `program_info/niijima/niijima-logo.svg` 及 PNG、多尺寸 ICO；品牌构建使用它作为 Windows 主程序、NSIS 安装/卸载器与 Qt 应用默认图标，快捷方式跟随主 EXE。原 Prism 构建资源保留。窗口实际显示继续等待用户允许可视化验证。

更名后验证：NiijimaLauncher Release 增量构建、全新 install-niijima 收集及 NSIS 编译成功；安装器 30104174 字节已交付到 D:\mydata\Agents\Minecraft相关\本地资源\启动器安装器\2026-10-29\NiijimaLauncher-Setup.exe。旧 AshBill 安装器移入同目录的历史子目录。没有运行安装器或启动器；本轮为名称、标识和图标变更，沿用上轮 22/22 测试，不重复无关测试。日志位于仓库外工具/prism-build/niijima-rename-*.log。

构建内存注意：本次 C1002/LNK1257 已通过 /CGTHREADS:1 + 单并行构建解决，命令与 MSVC /GL 行为说明见安装器专题。git diff --check 通过。

## 2026-10-29 署名、GPL 通知与后续提交授权

本轮仅编辑：关于页源码链接指向 origin fork；作者页前加 AshBill / GPT-6 Astra（AI 开发协作），上游 Prism 作者名单标题恢复为 Prism Launcher Developers；原 credits.html、LICENSE、COPYING.md 与上游版权字符串完整保留。发行 README 新增 `program_info/niijima/README-runtime.md`，CMake Windows install 和打包脚本均会将 README.md、LICENSE、COPYING.md 放在程序旁；仓库首页增加本分支说明。

仅做静态核查，**未构建、未 commit、未 push**。当前链接由本地 origin 核对；尚未确认远端含本轮源码。未来发布需要用户 push 对应源码并明确对应 commit/tag；尤其不能将构建前旧 HEAD 误称为含未提交修改的完整源码版本。用户指定顺序是明确要求构建 → 构建/验证 → 分类 commit → 提醒手动 push；不要擅自为更新嵌入 commit ID 再构建。

现有测试目录已直接补入 README.md、LICENSE、COPYING.md（普通文档复制），EXE 与 DLL 未改动；这不属于构建或重新打包。


## 2026-10-29 分离界面图标及 EXE 名称（未构建）

- 用户明确区分两套图标：EXE、安装器、程序快捷方式及 Niijima 开屏使用橙白 N；“进入 Prism”按钮、Prism 主窗口、关于页及默认 Prism 对话框使用原 Prism 图标。
- `Application::logo()` 保留 Niijima 品牌图标；新增 `prismLogo()` 从单独嵌入的原始 SVG 加载。Qt 默认图标为 Prism，Niijima 开屏显式设置 N 图标。创建实例快捷方式时恢复操作前的全局图标，避免把默认图标重置为 N。实例自己的图标仍按原逻辑处理。
- 下次构建主程序改为 `install-niijima/niijima-launcher.exe`，辅助程序相应为 `niijima-launcher_filelink.exe`，URL 协议随二进制名变为 `niijima-launcher://`。产品/数据目录/应用 ID/卸载键保持 NiijimaLauncher；配置文件特意保留 `niijimalauncher.cfg`，继续读之前的设置。
- 现有旧测试 EXE、DLL 和安装器未改动。打包脚本仅略过旧文件名的两个预览 EXE，不删除它们；必须先存在新主程序和辅助程序才允许打包，避免新旧二进制同时进入安装器。
- 未配置、未编译、未打包、未运行 GUI、未 commit/push。图标在实际窗口及 Windows 任务栏的显示需下次明确构建后验收。


## 2026-10-29 用户截图定稿（本次已明确授权构建）

- 顶栏改中文“新岛启动器 v1.0”，使用 Noto Sans SC；启动器版本与整合包版本分开。
- 英文 NIIJIMA RAILWAYS 移入左侧标题组；新增四字白色 SVG 和“==しゅっぱつしんこう！！==”日文标语。SVG 来自当前内置 Zen Maru Gothic Bold，只去掉鉄右部上方竖笔出头，其余轮廓不变；普通日文仍用字体。资源为 home-title.svg，生成脚本 tools/niijima/generate_home_title.py，OFL保留。
- 主按钮改为28px粗体白字；启动按钮左侧显示所选已安装整合包版本，无实例时隐藏。优先读取实例 mrpack/modrinth.index.json 的 versionId，再用ManagedPack版本名；保留完整版本并按需补v，读不到显示未知，不猜测MC版本或写死v1.6。切换实例/导入完成后重读。
- 同时构建此前待生效的图标分离、niijima-launcher.exe改名、署名与源码链接。用户已明确要求本次构建，随后分类commit；仍不push或擅自显示GUI。构建结果另补。


## 2026-10-29 截图 UI 构建结果

- Release 主程序、filelink 辅助程序和 LauncherTests 已成功编译；CTest 22/22 通过（排除曾触发弹窗的 FileSystem），镜像工具离线测试 3/3 通过。无安装/启动/账号/游戏运行的实际验收。
- 当前测试入口：`install-niijima/niijima-launcher.exe`，37,696,512 字节。已包含中文顶栏v1.0、矢旁四字字标、指定日文标语、28px主按钮、真实已安装整合包版本、图标分离、AshBill/GPT-6 Astra署名和fork源码链接。
- NSIS 安装器已成功生成并复制到 `D:/mydata/Agents/Minecraft相关/本地资源/启动器安装器/2026-10-29/NiijimaLauncher-Setup.exe`，30,116,886 字节；前一安装器已移入同目录历史目录。未运行安装器或显示启动器窗口。
- 原无横线预览 EXE 仍留在运行目录供恢复；它不再代表最新版本，打包脚本明确不将它和旧 filelink 包进安装器。当前程序旁 README/LICENSE/COPYING 已更新。
- 本轮源码检查与构建日志在仓库外 `工具/prism-build/ui-v1-*.log`。页面布局仍待用户打开最新版做视觉/交互验收。
- 按已获授权，将未提交工作按镜像工具及启动器/UI/品牌安装器两组提交，再提醒用户手动 push。构建发生于提交前，“关于”页旧 HEAD 是构建时元数据，不能当作完整新源码版本；对应最终源码应以此次提交为准。

提交前格式核查：保留两份上游 OFL 原文中的行尾空格，diff --check 仅对这两份原文作路径豁免，其余修改通过检查。

## 2026-10-29 默认镜像与 DPI 修复构建完成

- 用户明确授权：一个 tar.gz 上传到 `minecraft/niijima-launcher/` 后原地解压；启动器内置对应镜像并构建。最终包为仓库外 `本地资源/Prism镜像部署/上传即用-20261029/niijima-mirror.tar.gz`（673,669,096 字节），自带 `niijima-mirror/assets/`、`files/`、`meta/`。不再采用旧 `publish/` 或元数据补丁流程。
- `Application.cpp` 仅对 AppID `com.ashbill.NiijimaLauncher` 注册 `MetaURLOverride` / `ResourceURLOverride` 默认值，分别为 `https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/niijima-mirror/meta/v1/` 和 `https://files.yake.wang:25572/%E5%88%9B%E4%B8%96%E7%BA%AA%E7%BD%91%E7%9B%98/minecraft/niijima-launcher/niijima-mirror/assets/`。下载代码未改，copyparty 可直接按文件路径提供，不需要新增 nginx 路由。
- 新配置或缺少镜像键的旧配置自动使用上述默认值；已保存的自定义地址保留，显式空值仍使用官方原源。`BuildConfig` 官方 fallback 未改。检查到本机预览配置原本没有镜像键，因此无需替用户改配置文件；没有写入用户配置或清除其缓存。
- 同时包含 `QIcon::paint` 字标 DPI 修复、34 px 主按钮文字、2 px 字间距及加宽按钮。资源字形未改。
- Release 主程序和 filelink 编译成功，安装目录 `D:/mydata/Repositories/PrismLauncher/install-niijima/niijima-launcher.exe`（37,698,560 字节）；Windows NSIS 安装器 `D:/mydata/Agents/Minecraft相关/本地资源/启动器安装器/2026-10-29/NiijimaLauncher-Setup.exe`（30,114,027 字节）已更新，旧安装器留在同目录“历史”。二进制内已确认最终无 publish 的镜像根和 34 px 样式字符串。
- CTest 22/22 通过（排除 FileSystem），镜像工具测试 3/3 通过；最终归档元数据的 7 个 JSON、SHA256 链、174 个 URL 已核对。归档按 7z 成功退出验收，没有重复解包校验。
- 本轮没有运行启动器或安装器 GUI；高 DPI 外观、真实安装/登录/进游戏仍未实测。新镜像包等待用户上传并解压，不能把本地构建当作线上可用。正式 pack.zip 及其下载 URL 仍需另行配置。
- 日志：仓库外 `工具/prism-build/mirror-final-*.log`。PowerShell 5 会把 CMake 警告误当终止错误；改用 `pwsh`（PowerShell 7）后构建正常。保留 `/CGTHREADS:1`，应用/辅助程序并行4，测试链接单并行，未运行 FileSystem 测试。
- 按用户既有授权在构建完成后分 UI 与镜像两组提交；不 push，需用户自行公开对应源码。构建早于这两个提交，程序内 Git HEAD 仍是构建时值，不能代表尚未写入的提交。

补充无窗口渲染检查：使用 Qt 6.11.2 的 offscreen 平台和当前 SVG，在 DPR 1、1.5、2 下按 HomeWordmark 的 444×110 / QIcon::paint 绘制；三种缩放的四字均完整，透明四边未触达，PNG 已目视核查。此检查验证字标绘制，不替代完整启动器布局/交互验收。PNG、results.json 和 helper 位于仓库外 `工具/prism-build/title-render-regression/`；未打开任何窗口。

## 2026-10-29 Regular 外观与数字版本索引构建完成

- 用户明确要求“构建”后执行。Release 主程序、filelink 和 LauncherTests 编译成功；`install-niijima/niijima-launcher.exe` 为 **41,542,144 字节**，已确认二进制包含 `packs/latest.json`、`NiijimaPackVersionId` 和 Regular 字体资源入口。
- Windows 安装器已生成到仓库外 `本地资源/启动器安装器/2026-10-29/NiijimaLauncher-Setup.exe`，**31,902,037 字节**；前一安装器保留在同目录“历史”。未执行安装器或打开启动器窗口。
- **CTest 23/23 通过**，包含新 `NiijimaPackIndex` 测试，覆盖数字最大 ID、指定 ID、quint64 边界、中文/空格及字面转义文件名、非法路径和 JSON。继续排除已知会触发弹窗的 FileSystem。未重复运行未改动的镜像 Python 测试。
- 独立 Qt offscreen 检查覆盖 960×600 / 780×510、DPR 1 / 1.5 / 2，以及下载/启动文案：无裁切，实际字距18 px，左右墨迹留白差≤0.67逻辑像素，两个按钮齐底；36 px 主按钮实测约242×81逻辑像素。副标题实际加载 Zen Maru Gothic Regular / OS2 weight400；46 px顶栏、18 px标题组间距、统一blur18无遮罩均取当前源码。
- offscreen 仅验证仓库外提取的当前视觉组件和布局，没有连接实际账号、下载或版本状态；不能视作完整启动器交互验收。PNG/记录位于仓库外 `工具/prism-build/home-polish-regression/`。构建/CTest/安装器日志为 `工具/prism-build/pack-index-ui-*.log`。
- 正式索引部署规则见《整合包版本索引》：上传 `packs/latest.json` 及它指向的 `新岛铁道 v1.6.zip`。本轮仅核对本地索引源与交付副本一致，没有上传或验证线上索引/包，未实际安装游戏。固定镜像归档不需重打。
- 按既有授权，构建完成后分字体资源、首页与版本索引两组本地提交。源码提交晚于构建，程序内嵌 Git HEAD 为构建时值；发布前由用户 push 对应源码，agent 不 push。
