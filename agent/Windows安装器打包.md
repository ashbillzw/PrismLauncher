# Windows 安装器打包

## 当前迭代约定（2026-10-29 用户明确要求）

- 修改后先保留源码/文档，**只有用户明确指示构建时才配置、编译、收集运行目录或重打安装器**；未构建的回复应明确写“未构建”。不要因代码修改自动触发 CMake/Ninja/NSIS。
- 用户明确构建后，完成适用验证，再按功能/依赖关系写一个或多个本地 commit，并提醒用户将对应源码 push 到公开仓库。用户已授权这次后续构建完成后的分类提交；展示分组 diff 与验证结果，不重复索取相同授权。未相关的其他工作另行判断。
- Agent 不执行 push，不创建远端发布。源码 URL 为 `https://github.com/ashbillzw/PrismLauncher`；commit 不等于 push，源链接存在不等于云端已含本地修改。
- 当前可直接运行的成品为 `install-niijima/niijima-launcher.exe`（2026-10-29 截图 UI 版），与 DLL、Qt 插件等配套使用。本轮截图、署名、文件名和图标分离已包含；暂不自动显示启动器或安装器窗口。


更新：2026-10-29。用户澄清 AshBill 只是为了避免原版冲突，产品统一命名为 NiijimaLauncher。已走通 NiijimaLauncher 独立 Release 构建、运行目录收集及 NSIS 安装器生成。**只编译，没有执行安装器或修改本机注册表；安装/卸载及 GUI 尚未实测，安装器未签名。** 2026-10-28 仅研究 NSIS / NScurl 的方案已由本页替代。

## 当前产物与隔离

- 可执行程序：`install-niijima/niijima-launcher.exe`（本次已构建）。
- 安装器：`build-ashbill/dist/NiijimaLauncher-Setup.exe`，通过 `tools/niijima/package_windows.ps1` 生成。
- 交付副本：`D:/mydata/Agents/Minecraft相关/本地资源/启动器安装器/2026-10-29/NiijimaLauncher-Setup.exe`。
- 构建、安装目录及 dist 全部由 Git 忽略；没有自动提交/推送/部署。

| 项目 | NiijimaLauncher 版本 |
| --- | --- |
| 名称 | NiijimaLauncher |
| EXE | `niijima-launcher.exe` |
| 默认安装目录 | `%LOCALAPPDATA%\Programs\NiijimaLauncher` |
| 默认数据目录 | `%APPDATA%\NiijimaLauncher`（源码派生值；未实际安装验证） |
| 配置文件 | `niijimalauncher.cfg` |
| 应用 ID / Windows AppUserModelID | `com.ashbill.NiijimaLauncher` |
| 安装登记 | `HKCU\Software\NiijimaLauncher` |
| 卸载登记 | `HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\NiijimaLauncher` |
| URL 协议 | `niijima-launcher://` |
| 开始菜单 | 当前用户的 `NiijimaLauncher` 文件夹 |

不注册 `curseforge://`、`modrinth://`、`prismlauncher://`，不修改 `.mrpack` / `.zip` 的默认关联；已有安装目录中检测到官方 `prismlauncher.exe` 会拒绝混装。单实例 IPC 在 fork 中加入独立应用 ID，避免与官方进程误通信。普通 Prism 的配置和数据没有迁移或修改。

`Launcher_UPDATER_GITHUB_REPO` / `Launcher_BUILD_ARTIFACT` 均为空，`Launcher_BUILD_UPDATER=OFF`，因此不会从官方 Prism 更新源自动覆盖本版本。自有更新发布流程、签名和账号登录验证仍需后续完成。

安装器按用户安装，无管理员请求；VC++ 运行库采用本机 VS 的可再分发 app-local CRT，随程序放在安装目录里，不运行全局 VC redist 安装、不依赖 NScurl 下载。卸载清单只列本包文件，目录仅用非递归 RMDir 清理空目录，保留用户数据及不属于本包的文件；注册表删除前核对当前归属。

## 本机构建

工具：VS 2022、Qt 6.11.2 MSVC x64、JDK 17、CMake/Ninja、NSIS 3.13。`niijima` preset 使用独立 `build-ashbill` / `install-niijima`，可继续保留原 default 构建。

```powershell
& 'D:\Program Files\Microsoft\Visual Studio\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
$env:JAVA_HOME='C:\Program Files\Eclipse Adoptium\jdk-17.0.16+8'
$env:VCPKG_VISUAL_STUDIO_PATH='D:\Program Files\Microsoft\Visual Studio'
$env:VCPKG_MAX_CONCURRENCY='4'
cmake --preset niijima -D CMAKE_EXE_LINKER_FLAGS="/CGTHREADS:1" -D CMAKE_PREFIX_PATH='D:/mydata/Tools/PrismBuild/Qt/6.11.2/msvc2022_64' -D VCPKG_TARGET_TRIPLET=x64-windows -D VCPKG_HOST_TRIPLET=x64-windows -D VCPKG_INSTALLED_DIR='D:/mydata/Repositories/PrismLauncher/build/vcpkg_installed'
cmake --build --preset niijima --config Release --parallel 1
cmake --install build-ashbill --config Release
```

逐步检查退出码，失败即停止；上述 vcpkg 路径用于本机复用已安装依赖，其他机器可省略该参数使用独立依赖目录。正式打包前确认没有遗漏本轮源码变更。

```powershell
& .\tools\niijima\package_windows.ps1 `
  -NsisCompiler 'D:\mydata\Agents\Minecraft相关\工具\prism-build\nsis-toolchain\extracted\nsis-3.13\makensis.exe' `
  -VCRuntimeDir 'D:\Program Files\Microsoft\Visual Studio\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT'
```

脚本从独立运行目录复制白名单运行文件到新的 staging，附项目及字体许可证、app-local CRT，按实际文件生成卸载清单；用 CMake 生成的 `build-ashbill/program_info/win_install.nsi` 编译。没有运行安装器或启动器的步骤。实际模板为 `program_info/win_install_niijima.nsi.in`；原上游模板仍供默认构建使用。需要重新打包时再次运行脚本即可。

## 工具来源与验证范围

NSIS 官方：[下载页](https://nsis.sourceforge.io/Download) / [3.13 ZIP 页面](https://sourceforge.net/projects/nsis/files/NSIS%203/3.13/nsis-3.13.zip/download)。便携工具只解压到仓库外，不安装到系统。普通网页及其部分下载中转返回 HTML；本次最终通过 curl 无代理访问 master.dl.sourceforge.net 的官方 ZIP 路径取得归档，7z 解压成功，makensis /VERSION 确认 v3.13。

- Release 全量 477 步编译及 install 收集成功。
- NSIS 编译生成标准 MUI 安装器（欢迎、组件、目录、安装、完成，及卸载界面），包含简体中文、英文、日文语言表。
- 按源码与生成配置核对独立品牌、数据路径推导、注册表/协议/快捷方式、禁用官方 updater 与卸载清单；不把静态核查当成实际安装测试。
- 未签名；没有运行安装器、没有实际写注册表、没有再打开启动器窗口。镜像真实 URL、真实整合包导入、登录与游戏启动仍待后续验证。
- 日志位于仓库外 `工具/prism-build/ashbill-configure.log`、`ashbill-build.log`、`ashbill-install.log`、`ashbill-package.log`。

### 2026-10-29 最终验证补充

- `ctest --test-dir build-ashbill -C Release -E FileSystem --output-on-failure`：22/22 通过。为遵守不显示窗口的要求，排除历史上曾触发 filelink 错误弹窗的 FileSystem；没有运行 GUI。
- NSIS 显式使用 `/INPUTCHARSET UTF8`，重新编译成功；最终安装器 30,437,181 字节，已复制到上述仓库外交付目录。`git diff --check` 通过（只有 Git 的 LF/CRLF 提示）。
- NSIS 3.13 归档本次官方 SourceForge 路径曾重定向到 HTTP 镜像，因此另核对了 HTTPS 包维护记录：本地 SHA256 为 `BA63DFFC4410EE89193E1CB5A41989991BD77C61068DA17E3156D136B7B0B3D8`，与 [Chocolatey 3.13.0](https://community.chocolatey.org/packages/nsis.portable/3.13.0) 一致；SHA1 为 `DB14F8AF2EA346B786C6AE19343F61AAF5E09B8E`，与 [Scoop Extras nsis 3.13 manifest](https://raw.githubusercontent.com/ScoopInstaller/Extras/master/bucket/nsis.json) 一致。这是维护者记录核验，不是直接读取 NSIS 上游 SHA256。


### NiijimaLauncher 命名更正

- 显示名、安装器文件、数据目录及注册表统一使用 NiijimaLauncher；二进制/cfg/协议使用全小写 niijimalauncher，以兼容现有 QUrl scheme 比较。
- AppID `com.ashbill.NiijimaLauncher`、域名 `ashbill.com` 和 Publisher `AshBill` 保留为开发者身份，不是产品名前缀。
- `niijima` preset 继续复用历史 `build-ashbill` 编译缓存目录（名称只在本机开发过程中出现），安装收集改用全新 `install-niijima`，不混入旧二进制。
- 上述 477 步、22/22 和 30,437,181 字节为本次更名前的验证记录；更名后构建/打包结果见本节后续记录。历史日志仍保留 ashbill 前缀。


## 2026-10-29 自有标志

用户指定群图标 https://p.qlogo.cn/gh/549428882/549428882/0 作为 EXE 等处 logo 的基础。橙白 N 标志已整理为 `program_info/niijima/niijima-logo.svg` 及 PNG、多尺寸 ICO；品牌构建使用它作为 Windows 主程序、NSIS 安装/卸载器与 Qt 应用默认图标，快捷方式跟随主 EXE。原 Prism 构建资源保留。窗口实际显示继续等待用户允许可视化验证。

更名后验证：NiijimaLauncher Release 增量构建、全新 install-niijima 收集及 NSIS 编译成功；安装器 30104174 字节已交付到 D:\mydata\Agents\Minecraft相关\本地资源\启动器安装器\2026-10-29\NiijimaLauncher-Setup.exe。旧 AshBill 安装器移入同目录的历史子目录。没有运行安装器或启动器；本轮为名称、标识和图标变更，沿用上轮 22/22 测试，不重复无关测试。日志位于仓库外工具/prism-build/niijima-rename-*.log。

更名/图标构建补充：首次链接遇 MSVC C1002（pass 2 堆空间不足）/LNK1257；使用 `/CGTHREADS:1` 和 `--parallel 1` 后构建、NSIS 打包成功。根 CMake 的 MSVC 分支仍手工添加 `/GL` 和 `/LTCG`，所以当前 `ENABLE_LTO=OFF` 并不能关闭它们；本次没有扩大修改该上游逻辑。新图标 ICO 包含 16/20/24/32/40/48/64/128/256 px，SVG 为纯填色路径。`git diff --check` 通过，未实际运行 GUI。复现日志为 `niijima-rename-resume-configure.log`、`niijima-rename-resume-build.log` 和 `niijima-rename-package.log`。

## 署名与源码分发（2026-10-29，待下次明确构建）

NiijimaLauncher 署名为 AshBill 和 GPT-6 Astra（AI 开发协作）。源码链接指向 https://github.com/ashbillzw/PrismLauncher；原始 LICENSE、COPYING.md、credits.html 和 Prism/PolyMC/MultiMC 版权信息未删改。打包脚本与 CMake install 已接入程序旁的 README.md、LICENSE、COPYING.md；原 licenses/ 中的通知及字体 OFL 继续保留。

本轮没有运行配置、编译、安装收集或 NSIS；现有二进制仍是署名改动前版本。用户明确构建后再分功能 commit，提醒用户手动 push 到公开 GitHub。发布时需在下载说明中标出对应源码版本，不能只链接到缺少本地修改的旧源码。GPL 对外分发的对应源码要求见 [GNU FAQ](https://www.gnu.org/licenses/gpl-faq.en.html#GPLRequireSourcePostedPublic) 和仓库 LICENSE 第 6 节。

现有测试目录已直接补入 README.md、LICENSE、COPYING.md（普通文档复制），EXE 与 DLL 未改动；这不属于构建或重新打包。


## 2026-10-29 分离界面图标及 EXE 名称（未构建）

- 用户明确区分两套图标：EXE、安装器、程序快捷方式及 Niijima 开屏使用橙白 N；“进入 Prism”按钮、Prism 主窗口、关于页及默认 Prism 对话框使用原 Prism 图标。
- `Application::logo()` 保留 Niijima 品牌图标；新增 `prismLogo()` 从单独嵌入的原始 SVG 加载。Qt 默认图标为 Prism，Niijima 开屏显式设置 N 图标。创建实例快捷方式时恢复操作前的全局图标，避免把默认图标重置为 N。实例自己的图标仍按原逻辑处理。
- 下次构建主程序改为 `install-niijima/niijima-launcher.exe`，辅助程序相应为 `niijima-launcher_filelink.exe`，URL 协议随二进制名变为 `niijima-launcher://`。产品/数据目录/应用 ID/卸载键保持 NiijimaLauncher；配置文件特意保留 `niijimalauncher.cfg`，继续读之前的设置。
- 现有旧测试 EXE、DLL 和安装器未改动。打包脚本仅略过旧文件名的两个预览 EXE，不删除它们；必须先存在新主程序和辅助程序才允许打包，避免新旧二进制同时进入安装器。
- 未配置、未编译、未打包、未运行 GUI、未 commit/push。图标在实际窗口及 Windows 任务栏的显示需下次明确构建后验收。


## 2026-10-29 截图 UI 构建结果

- Release 主程序、filelink 辅助程序和 LauncherTests 已成功编译；CTest 22/22 通过（排除曾触发弹窗的 FileSystem），镜像工具离线测试 3/3 通过。无安装/启动/账号/游戏运行的实际验收。
- 当前测试入口：`install-niijima/niijima-launcher.exe`，37,696,512 字节。已包含中文顶栏v1.0、矢旁四字字标、指定日文标语、28px主按钮、真实已安装整合包版本、图标分离、AshBill/GPT-6 Astra署名和fork源码链接。
- NSIS 安装器已成功生成并复制到 `D:/mydata/Agents/Minecraft相关/本地资源/启动器安装器/2026-10-29/NiijimaLauncher-Setup.exe`，30,116,886 字节；前一安装器已移入同目录历史目录。未运行安装器或显示启动器窗口。
- 原无横线预览 EXE 仍留在运行目录供恢复；它不再代表最新版本，打包脚本明确不将它和旧 filelink 包进安装器。当前程序旁 README/LICENSE/COPYING 已更新。
- 本轮源码检查与构建日志在仓库外 `工具/prism-build/ui-v1-*.log`。页面布局仍待用户打开最新版做视觉/交互验收。
- 按已获授权，将未提交工作按镜像工具及启动器/UI/品牌安装器两组提交，再提醒用户手动 push。构建发生于提交前，“关于”页旧 HEAD 是构建时元数据，不能当作完整新源码版本；对应最终源码应以此次提交为准。

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
