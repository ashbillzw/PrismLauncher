# PrismLauncher 交接

## 2026-10-31 首页手动最小化按钮已实现并交付

- 用户明确要求立即完成此前缺失的最小化按钮。右上角关闭按钮左侧新增“−”，宽 62px、高度沿用标题栏；使用白色符号、透明背景和轻微悬停/按下反馈，直接连接 QWidget::showMinimized。窗口原有尺寸及主体布局不变。下方“最小化按钮仍未实现”仅为历史状态，已被本节替代。
- Release 构建成功，install-niijima/niijima-launcher.exe 已替换：41,589,248 字节，2026-10-31 02:39:21 +08:00；SHA256 6EA50D0E0F6649E0D3A7BB8E7D91DD3FE96053C5B7908038A8D04E2B931994E9，与 Release 产物一致。此处是原生窗口槽连接的小改动，没有增加或重复运行无关回归测试；安装器和整合包未重打。
- 用户再次明确：未授权 computer use，不应从“实现功能”推断 GUI 操作授权。本轮仅初始化了工具、查询应用列表，随后停止；没有通过工具启动、点击或操作启动器，也没有实际点击验证。后续如需 computer use，先取得用户明确授权。按钮点击和从任务栏恢复由用户验证，状态已同步整合包总 todo，不把编译通过说成 GUI 测试通过。
- 可见构建以 Minimized 启动，任务结束后由既有包装器倒计时关闭。构建/安装日志、旧 EXE 备份位于仓库外 prism-build/，前缀 home-minimize-20261031。测试 JSON 仍无 testWarning，空字符串关闭警告的前轮修复保留。

## 2026-10-31 02:31 测试警告关闭、最小化原话与构建窗口重测

- 当前 install-niijima/niijima-local-test.json 已删除 testWarning，保留 packs/mirror。LocalTest 解析器新增：testWarning 值为空字符串时不触发测试警告；缺键同样不触发，非空字符串显示原文。原有非字符串 true/false/null 的“键存在即触发”行为未在本次改变；真实低内存警告不受这个测试开关影响。
- Release 构建、NiijimaLocalTest 回归通过（含新增空字符串关闭测试）；测试 EXE 已替换，41,588,224 字节，2026-10-31 02:29:50 +08:00，SHA256 0071F7C3868ADC2A7D9105C3E70938953AB723E5EFF263A8BBE3447EF29A0998，与 Release 产物一致。未实际打开 GUI。下方 02:15 的 EXE/启用测试文案状态已被本节替代；安装器未重打，仍为该节产物。本节初写时源码、测试及文档尚未提交（历史状态）；2026-10-31 后续按用户要求提交：警告修复及测试为 db0a64395，规则与交付文档另组提交。验证沿用上述构建结果，不重复编译。
- 聊天原话核实：在「（编程1）更新助手指令偏好」2026-10-30 04:44:06 +08:00，用户说“我发现你的 控件根本没有最小化按钮。今天太晚了先到这吧。没做完的都写到todo，然后commit work，然后fetch pull merge。”当时按停工要求记录到整合包 agent/todo-list.md 的“补首页手动最小化按钮”；仍未实现。本轮只核实，不将询问扩大为控件开发。
- 弹窗确实复用 CustomMessageBox::selectable（内部 new QMessageBox）；其 parent 为首页，继承首页通用 QPushButton 的 border-radius:10px 等样式。用户明确接受此继承外观，本轮不改样式。不能把“复用实现”说成“外观与原生 Prism 完全相同”。
- 默认最小化重测：最初观察到展开/前台变化，但未记录输入，用户表示可能手动点开，因此不能据此认定 Windows Terminal 忽略参数。成功重测的窗口归属进程为 powershell；这不等于已验证直接 wt.exe 启动路径，两者不要混称。02:29:51–02:30:17 使用原始 Start-Process powershell.exe -WindowStyle Minimized 路径重测，同时记录 GetLastInputInfo：没有输入活动，测试窗口始终最小化，任务 3 秒加倒计时 15 秒后退出。期间系统前台句柄仍有变化，故只确认“默认最小化、不自动展开”，不宣称完全不改变键盘焦点。传统控制台/不激活模式的探索没有应用到构建流程。无实际关闭既有终端操作。
- 本轮增量构建也使用 Minimized 启动和既有 15 秒关闭包装器。日志前缀 prism-build/build-empty-warning-20261031；配置备份 niijima-local-test-before-warning-disabled-20261031.json；最小化观测 minimized-window-test.json 均位于仓库外工作目录。没有重复构建整合包或修改发布 pack-id。

## 2026-10-31 02:15 最大 pack-id 与单文件元数据版已交付

本节替代下方 01:59/首页取消选择框章节的“源码未构建、未替换 EXE、测试未执行”状态；旧双文件、source、旧测试兼容方案仍为过时历史。

- 首页无实例下拉框；扫描本地有效 token 和 pack-id，启动数值最大者。版本显示、启动与更新判断共用选择结果；不依赖显示名、旧偏好或在线版本映射。仅使用 niijima-pack.json 的 token 与 pack-id；不保存 source/重复 version，不读写 niijima-release.json，不兼容旧测试格式。Prism 原生 mmc-pack.json 保持不变。
- Release 编译、部署成功；CTest IconUtils、NiijimaInstalledPacks、NiijimaPackIndex、NiijimaLocalTest 共 4 组通过。覆盖数字最大 ID、同 ID 稳定选择、非法 ID/删除实例、标记写入，以及既有更新状态、本地传输/索引、图标逻辑。未运行游戏、未自动创建实例、未做 GUI 验收。
- 测试 EXE：C:/mydata/Repositories/PrismLauncher/install-niijima/niijima-launcher.exe，41,587,712 字节，2026-10-31 02:13:03 +08:00；SHA256 D630DA56A67A7F69463A0F7D4B18F85C8C8A86FDE5F4F6B2A3115AA75F038FA1，与 Release 产物一致。原测试 JSON 的 packs/mirror/testWarning 保留。
- 安装器：build-ashbill/dist/NiijimaLauncher-Setup.exe，31,944,682 字节；交付副本 C:/mydata/Agents/Codex-CreateTrainResync/本地资源/启动器安装器/2026-10-31/NiijimaLauncher-Setup.exe。两者 SHA256 均为 6FD8D8ED6946C671398B173B764E1C684E2C0D9771136367FF807E708623305C；45 项 payload 不含测试 JSON、账号或实例。安装器未执行、未签名、未上传。
- 整合包内容未改，本轮不重复运行 build.bat/build-test.bat；仍用已交付 test0004、pack-id 3，未递增 test 编号或发布 ID。TrainResync 无改动，不重复编译。
- 可见终端使用 tools/niijima/run_visible_build.ps1；任务完成 15 秒关闭，按任意键进入交互提示符保留。启动不能加 -NoExit：首次尝试虽倒计时但仍回到提示符，已纠正。修正后独立可见检查窗口及实际安装器打包进程均自动退出；按键保留分支尚未实际输入验证。用户未授权清理既有窗口，本轮未操作/关闭旧窗口。
- 首次构建被 WinPS5 对 native stderr 的 Stop 处理打断，改为 cmd 合并输出后再传给 Tee-Object，仍检查退出码；第二次编译成功，无降低编译警告级别。临时打包脚本中文路径因 UTF-8 无 BOM 被 WinPS5 误读，已改为 UTF-8 BOM，并将成品复制到正确交付目录，哈希核对通过；未因此重新编译。
- 构建、测试、安装及打包日志/旧产物备份位于仓库外 prism-build/，本轮前缀 pack-selection-20261031。新功能、构建工具/规则与交付文档分别提交；不 push。构建早于提交，内嵌 Git HEAD 可能滞后，不为此重复构建。

## 2026-10-31 01:59 最终简化：单文件pack-id，不兼容旧测试格式（源码未构建）

- 用户随后纠正：本地测试阶段无需旧包/旧字段兼容；不需要保存source；自定义两个JSON应合并。**本节替代紧随其后的“旧id兼容/保持两个JSON/版本映射/首选实例平局”等本轮早期方案，历史不删除。**
- 只使用实例根niijima-pack.json：`{"token":114514,"pack-id":"3"}`。pack-id按十进制字符串完整保存quint64；最新包版本文字从mrpack/modrinth.index.json读取，不在自定义JSON再存version/source。安装完成后QSaveFile原子写pack-id到原token标记文件；写失败走已有安装失败状态，不冒充已完成首页登记。不生成/读取niijima-release.json，不识别旧id，不做迁移或索引版本文字推断。未登记pack-id的旧测试实例不参加首页候选。
- InstalledPacks扫描全部Prism实例，要求instance.cfg存在、token正确及pack-id为有效正整数，选数值最大者；同pack-id按实例目录ID稳定排序。不使用旧NiijimaInstanceIds/NiijimaPreferredInstance设置，两者注册与读写已删除。首页QComboBox/布局行/样式已删，版本文字与按钮共用选中实例，点击前重查。所有首页新安装统一通过发布索引确定pack-id，旧NiijimaPackUrl绕过索引的试验入口已删；普通Prism的手工ZIP导入功能不变。
- 新回归源码只覆盖当前格式：离线数字最大ID、不靠显示名称、排除非新岛标记、平局稳定/实例删除、非法ID、只写token+pack-id；旧兼容测试已撤换。静态调用链、残留检索、git diff --check通过；尚未编译或执行这些新增用例、未替换EXE、未提交。01:29测试EXE仍有旧下拉框/双文件逻辑，需下次明确构建后才生效。
- 用户追问的mmc-pack.json为Prism的PackProfile原生组件清单，记录Minecraft/Forge/LWJGL版本；Prism读取它进行依赖与启动配置，Minecraft游戏不直接读取。该文件与自定义token/pack-id元数据职责不同，本轮不合并、不改动。
- 原source字段只是正式请求URL，LocalTestNetwork当时仍将它映射到本地文件，并不表示测试ZIP走网络；它是agent额外记录，本轮按用户意见移除。实例元数据一次性本地整理在校验前发现test0004目录已不存在；随后instances目录列举为空，故未备份/改写/删除任何实例文件，不进行自动恢复或创建实例。
- 设计偏好已写本机及两仓库规则；具体源码/文档改动留在工作区。本轮不编译/提交，未给新产物分配pack-id，不修改已有打包索引ID3。

## 2026-10-31 首页取消选择框，自动选择最大pack-id（源码未构建）

- 用户反馈整行实例选择框过大且挤坏布局，明确本次不制作替代下拉框：首页根本不需要实例选择。已删除QComboBox、独立布局行、对应样式和下拉状态；不改变窗口原有960×600初始尺寸。相关设计偏好已同步本机AGENTS与两个仓库agent/AGENTS。
- 用户将latest.json数字发布ID统一称为**pack-id**，仍与testNNNN及Prism实例目录ID独立。当前实际test0004实例根niijima-release.json已保存旧字段id:"3"，不是没有保存。沿用此文件，不将同一ID重复写入token文件；后续新安装写pack-id字符串，读取兼容旧id。niijima-pack.json中的token:114514继续用于识别新岛包。未改用户实际实例文件。
- 新InstalledPacks选择逻辑扫描Prism实例列表全部实例，通过token标记识别（无标记的早期首页登记实例兼容），不靠显示名称匹配。离线直接读取有效receipt的pack-id，取最大值；只有同pack-id平局才沿用原偏好，否则以实例ID稳定排序。旧收据version与实际manifest不符时不采用陈旧ID；无收据时可用在线索引唯一版本映射，未知记0。不依赖网络请求成功才能识别本地最高pack-id。
- 版本标签、启动/控制台/更新状态和点击启动共用m_selectedInstance/m_selectedPackId；刷新和点击前重新选择，原PreferredInstance不再优先于更大pack-id。原生Prism仍可管理/启动其他实例；首页不新增选择UI，不删除旧实例。
- 新增NiijimaInstalledPacks回归源码：离线最大ID胜过旧偏好与版本文字、token排除同名假包、旧id兼容、同ID平局、陈旧receipt、旧实例无标记及删除实例。静态调用链和git diff --check通过；**本轮未编译/运行新增测试、未替换EXE、未提交**，已交付01:29 EXE仍有旧选择框。下方“已构建”仅指本节之前功能。

## 2026-10-31 01:31 首页更新版已编译、打包交付

- 用户要求全部编译/打包/分类提交。Release构建成功，CTest IconUtils、NiijimaPackIndex、NiijimaLocalTest共3组全部通过，覆盖首页超时/迟到/旧回调、发布ID不回退及共用本地索引/ZIP传输；GUI更新按钮、实际新实例安装与旧游戏同时运行尚待用户测试。首次编译因局部data遮蔽QWidget::data的C4458警告被/WX拒绝，改名receiptData后增量构建成功，没有降低警告级别。
- 测试入口仍为install-niijima/niijima-launcher.exe，41599488字节，生成时间2026-10-31 01:29:33 +08:00，SHA256=8D5DF4E12F087043C66908B719692370FEFB38416C0EEBABAD2B1D6BE555D469；与build-ashbill/Release中EXE一致。测试配置现只保留packs、mirror、testWarning，旧pack键已删除。无需关闭正在运行的实例，本轮未启动/修改游戏。
- build-test.bat与build.bat均成功：测试包1.6.test0004、589359541字节；普通包1.6.0、130961654字节。本机test计数现在4。已将新测试包登记为发布ID3，固定文件名新岛铁道 v1.6.test0004.zip，仓库外prism-local-test/packs中的latest.json与仓库维护源一致。ID2/test0003及其ZIP保留，下一发布ID必须大于3；test编号与发布ID仍独立。
- 本轮也补齐NSIS 3.13便携工具（官方ZIP SHA256与项目历史核验值一致）并生成安装器：build-ashbill/dist/NiijimaLauncher-Setup.exe，31955510字节，SHA256=201D02144EBDBAF9C292C7D08ED1232D884E00867F8AB16ED3330E93FEE4B528。交付副本在C:/mydata/Agents/Codex-CreateTrainResync/本地资源/启动器安装器/2026-10-31/NiijimaLauncher-Setup.exe，哈希一致。采用仓库外全新runtime-home-updates-20261031目录收集，45项payload清单不含测试JSON/实例/账号文件，许可证保留。未执行安装器、未签名、未上传；正式分发索引上一轮HTTP404状态未被本轮改变。
- 功能源码提交0c7c873ce；发布索引及交付文档另分组提交，整合包仓库仅记录本轮打包结果。构建早于提交，内嵌Git HEAD可能滞后，不能据此否定功能已编入。构建/打包/测试日志与旧EXE备份均在独立工作目录prism-build，前缀home-updates、home-updates-retry1；没有为元数据重复构建。
- **下方“源码完成未构建/未执行新增测试/旧pack键临时保留/当前最新ID2”等描述为历史，已由本节交付状态替代。** 下方曾讨论原地覆盖更新的分析已废弃；当前更新只新建实例并保留旧实例。

## 2026-10-31 首页检查更新与新实例安装（源码完成，未构建）

- 用户已确认首页状态：每次正常显示首页时灰色禁用“加载中”，不展示检查更新文案；检查成功立即解除等待，1000ms（PreciseTimer）到时也解除等待，网络请求继续，迟到的新版本仍切换“更新游戏”。已有实例且无新版时沿用“启动游戏”/游戏运行时“打开控制台”，无实例仍为“下载游戏”。失败只写日志，1秒后允许原有操作。旧请求的完成/超时回调用generation隔离；开始安装及关闭首页时取消检查。
- **用户纠正：更新是新建实例，绝不是覆盖旧实例。** 首页导入显式setCreateNewInstance(true)，经InstanceImportTask传至ModrinthCreationTask，在旧实例探测之前进入createInstance；普通Prism导入默认行为不改。新包成功提交才remember为首选，旧实例、存档、设置及正在运行的旧游戏不改动；同名文件夹由原生DirNameFromString生成唯一后缀。不添加“游戏运行时不能更新”的限制，也不自动搬迁旧数据。发现更新优先显示“更新游戏”，旧游戏仍可通过Prism控制台访问。
- **用户纠正：本地测试与正式流程只在文件获取层不同。** 删除首页local.pack直读分支。本地也调用同一PackIndexTask→数字ID选择/比较→InstanceImportTask→新实例提交；LocalTestNetwork将正式URL下packs/*映射到本地packs目录，将niijima-mirror/*映射到本地mirror。无效本地配置/缺文件不回退访问正式分发。原NiijimaPackUrl直链覆盖仍是显式例外：无索引则不能检查更新，不是本地测试默认路径。
- **发布ID全局递增，不因测试转正式而回退，且与testNNNN毫无数值绑定。** 新实例成功提交后写niijima-release.json（十进制字符串id、实际manifest versionId、source索引URL）。比较只用target.id > installedId；配置固定到更小ID不会给已有新版显示更新。旧实例无记录时以索引唯一version匹配ID，无法匹配/存在歧义时视为0，新建一次实例后得到明确ID。索引version只是显示标签，保存receipt不要求它等于包内versionId；receipt绑定实际manifest用于识别之后手动换包。
- 已在tools/niijima/deploy/packs/latest.json的原ID1后登记ID2 → 1.6.test0003 → 新岛铁道 v1.6.test0003.zip；公开正式索引本轮GET返回404，未上传。仓库外prism-local-test/packs有同一JSON与现有测试ZIP的固定文件名副本（589359541字节），不改build/test-sequence，也不重新打包。下次登记发布ID必须大于2；保留历史ID，不重用，不从test0003推导为ID3。
- install-niijima/niijima-local-test.json已增加packs目录，保留mirror/testWarning；旧pack键临时保留，让当前尚未替换的旧EXE仍能运行，新代码完全不读取pack键。新构建交付后可移除旧键。配置备份在仓库外prism-build/niijima-local-test-before-home-updates-20261031.json。源码、测试、文档未提交，EXE未编译/替换，游戏实例未更改。
- 新增回归源码覆盖1秒UI门限/迟到及过期结果、测试转正式的ID单调规则，以及同一正式URL/解析器通过本地网络映射读取索引和ZIP，保留原校验/缺文件失败检查。git diff --check及调用链静态审阅通过；**本轮未编译执行这些测试，不等于运行验收通过**。沿用用户“明确要求后才构建”的迭代约定。
- 下方“方案待确认”“复用原地更新”和旧EXE不检查更新均为历史；当前目标以本节为准，已交付EXE能力仍以00:40记录为准，不能把源码完成当成已交付。

## 2026-10-31 首页更新触发核查（方案待确认，未改源码）

- 用户明确指出每次进入首页都应检查整合包更新；此前“旧实例不会自动升级”是当前实现缺口，不是目标行为。NiijimaHomeWindow::primaryAction 在找到实例后直接 launch 并 return，只有无实例的下载路径调用 startReleaseLookup；其成功回调直接导入，尚未区分检查与安装。进入首页应独立触发检查，不能仅把现有下载函数挪到 show 时执行，否则会直接下载/导入。
- 已向用户发出选项卡：发现新版后主按钮改为“更新游戏”，点击后更新；或自动开始更新。用户回复前不实现依赖该选择的按钮文案和安装行为。每次进入首页检查本身不再待确认。
- 可复用 InstanceImportTask 的 original_instance_id → ModrinthCreationTask 既有更新路径；需明确指向当前实例，不能仅靠包名匹配。正式索引的数字发布 ID 当前未持久化；已有实例可读取 mrpack/modrinth.index.json 的 versionId，但迁移匹配和本地测试 ZIP 的版本检查仍需实现。游戏运行时不能替换实例。
- 原生更新会删除旧索引/override 清单对应文件再合并 staging，并非完整事务回滚；overrides 内的用户配置可能被替换，saves 路径有原生询问。接入前需核对这一边界，不宣称所有用户配置自动保留或失败完全无影响。未修改实例、未下载包、未构建、未提交。
- 历史方案《Prism开屏与镜像源码核查方案》当时第6节只纳入下载/启动/进入Prism，明确未纳入额外更新系统；该范围不再代表用户本次要求。下方00:40交付记录仍描述已交付EXE，尚无首页更新检查。


## 2026-10-31 00:40 图标/名称及自定义警告版已交付

- 用户要求把可打包、编译、提交的当前工作完成。Release编译成功，CTest IconUtils、NiijimaPackIndex、NiijimaLocalTest三组全部通过；涵盖原生/PCL图标优先级和路径约束、版本发布索引、自定义警告字符串、自动内存策略等。未实测GUI新导入或游戏启动。
- 用户关闭旧启动器后已安装至install-niijima/niijima-launcher.exe，41585152字节，文件时间2026-10-31 00:38:29 +08:00；SHA256=B68183CE060BE39513915037E2855ED4589A5A6FA4B33C8D177B38A779BDCFC8，与构建EXE一致。现已包含完整包名导入、常规/PCL图标查找、内存较少措辞及自定义测试正文。测试JSON保留pack-test.zip路径，testWarning为“这是一条测试警告。”。没有重新制作安装器。
- 整合包已重新运行build-test.bat和build.bat，测试包为1.6.test0003（589359541字节），普通包仍1.6.0（130961654字节），均含三处Logo。测试编号只在打包成功后递增，本机序号为3；本地JSON直读pack-test.zip，不需要发布ID。正式发布仍需上传包并手动维护packs/latest.json数字ID/version/file；本轮未操作服务器。现有已安装实例不会自动升级或自动改名/换图，要验证新版导入需导入新的测试包，不删除已有实例。
- 代码提交：6e51ff3af（自定义警告文本）、39e332f61（包名与图标导入）；整合包提交cdf3a9d（图标副本及打包交接）。本文件及测试说明另行提交。TrainResync仓库干净，本轮未无故重建。未push。
- 构建、测试、安装日志及旧EXE备份位于仓库外C:/mydata/Agents/Codex-CreateTrainResync/prism-build/，本轮前缀icons-warning-20261031、pack-icons-20261031；旧EXE为niijima-launcher-before-icons-warning-20261031.exe。首次包构建因PS7模块路径污染WinPS5失败，调整本次进程模块路径后成功；EXE安装一次因shell无cmake PATH未执行，改用构建缓存中的CMake绝对路径成功。未重复编译。
- **下方“源码更新未构建/测试未运行/EXE仍不含修复”为历史状态，以上交付结果取代。** 编译在本轮提交前进行，内嵌Git版本可能滞后，不能据此判断功能未编入。
## 2026-10-31 实例名称和图标导入已修源码（尚未构建）

- 用户要求修复Prism未读取PCL图标及首页漏设默认图标key。NiijimaHomeWindow::startImport现调用setIcon("default")，恢复原生图标扫描；同时去掉写死的setName("新岛铁道")。直接导入缺少名称时，InstanceImportTask::processModrinth通过既有Json接口读取索引name和versionId，使用完整name作为实例显示名，避免删掉硬编码后导入空名；已有用户指定名称不覆盖。
- 图标查找抽到IconUtils::findPackIcon，保留原生优先级：指定key→根icon.png→overrides/icon.png；未找到时再依次读取client-overrides、overrides、根目录中的PCL/Setup.ini。LogoCustom:True时使用Logo字段配置的相对文件路径，兼容反斜杠和CRLF；只允许引用对应层内实际存在且支持扩展名的图片，不读取包外绝对路径/URL/越界相对路径。图标安装仍复用Prism原有注册入口，不新增注册机制。
- 新增IconUtils回归测试源码并接入LauncherTests，覆盖原生图标优先级、PCL配置路径、关闭自定义/缺失文件及路径越界。静态调用链审阅及git diff --check通过；按既有迭代约定尚未编译/运行新增测试、未更换测试EXE、未提交。修复作用于后续导入；现有实例的name/iconKey尚未变更。整合包仓库先前新增的两份icon.png仍保留，PCL原图保留。
- **下文“实例列表名称与图标诊断（未修复）”是历史状态，源码修复以本节为准；00:06:57 EXE尚不包含本节和自定义测试正文修改。**
## 2026-10-31 实例列表名称与图标诊断（未修复）
- **来源与格式补充（2026-10-31）**：git blame定位硬编码setName来自4cbef888e1（2026-10-29首次首页实现，agent作者）；提交说明和方案没有记录用户要求固定实例名，不能编造当时动机，也不是Prism导入必需。Modrinth官方mrpack规范没有定义包图标字段/路径：https://support.modrinth.com/en/articles/8802351-modrinth-modpack-format-mrpack 。当前Prism源码已有根icon.png或overrides/icon.png的导入约定，PCL/Logo.png+Setup.ini是PCL自身配置，不是通用标准。用户表示没有通用标准时可沿用PCL；本轮仅解释，未实现图标后备查找或名称修复。

- 已安装 `Roaming/NiijimaLauncher/instances/新岛铁道/mrpack/modrinth.index.json` 的name为“新岛铁道 v1.6.test0002”，versionId为“1.6.test0002”；版本元数据没有丢失。instance.cfg却为name=新岛铁道、iconKey空。原因是NiijimaHomeWindow::startImport调用setName("新岛铁道")覆盖包名，列表读取实例名，而首页版本行另外读取索引versionId。
- 包内overrides/PCL/Setup.ini明确Logo:PCL\Logo.png、LogoCustom:True；Logo.png仍存在，且已导入实例minecraft/PCL/Logo.png（21973字节）。用户记忆中的PCL自定义图标配置属实；Prism的InstanceImportTask::installIcon只查找指定key、根icon.png、overrides/icon.png，不解析PCL配置。
- 另有首页接线遗漏：InstanceTask::m_instIcon默认空QString，startImport未调用setIcon("default")；processModrinth仅当key恰为default时自动查找包内图标，因此当前连原生查找也未执行，空key最终写入实例。仅添加icon.png仍不足，需同时修正该入口。本轮仅诊断及记录，未改安装逻辑、包或现有实例，不新增待办条目，未构建/提交。
## 2026-10-31 测试警告支持自定义正文（源码更新，尚未重新构建）

`testWarning` 现支持字符串作为测试弹窗正文，例如 `"testWarning": "这是一条测试警告。"`。本机测试JSON已改为该句，与实际低内存提示区分。实际低内存提示按用户意见改为“当前电脑内存较少（小于16GB），游戏可能不能正确运行。”。

兼容既有键存在语义：删除键关闭强制测试；true/false/null或空字符串保留默认正文；非空字符串原样显示，支持JSON换行。新增解析回归用例，未编译执行。本轮未构建/更新EXE、未提交；00:06:57的测试EXE仍只识别键存在，尚不会显示自定义正文。下方强制警告固定使用默认正文的记录为历史行为，待下一次构建交付后生效。

## 2026-10-31 自动内存与警告测试版已交付

- 用户明确要求编译、更新测试EXE并分类提交。Release完整构建成功；NiijimaLocalTest回归通过（QtTest 7项含初始化/收尾，0失败），覆盖曲线边界、自动忽略旧4096、手动恢复策略、低内存阈值、testWarning键存在语义和本地下载校验。未运行无关测试、未实测游戏或弹窗UI。
- `install-niijima/niijima-launcher.exe` 已更新，41,580,544字节，文件时间2026-10-31 00:06:57 +08:00；SHA256为 `EEFFDE2E04BDFB5313C50C5CE2F5B0F9462760191FD79ADA48D5FE585BA9A149`，与build-ashbill/Release对应EXE相同。保留既有测试JSON的pack、mirror及testWarning:true；用户重新启动即可强制弹警告。64GiB机器自动模式预期-Xms512m/-Xmx32768m，实际游戏参数待用户测试。未重制安装器、未改现有Roaming手动内存配置。
- 备份旧EXE及构建/测试/安装日志位于仓库外 `C:/mydata/Agents/Codex-CreateTrainResync/prism-build/`，文件前缀build/test/install-auto-memory-20261031；旧EXE为niijima-launcher-before-auto-memory-20261031.exe。命令从00:04:02至00:06:59完成，约2分57秒；结果确认等待另在任务阶段日志记录。
- 源码分类提交：584020d9f（控制台按钮黄黑斜纹及白字描边）、71e199fc5（自动内存与低内存/强制测试警告）。文档另行提交。构建早于本次提交，程序内嵌Git HEAD可能仍为旧值，不能据此否认功能已编入；本轮不为元数据再次构建。未push，由用户手动推送。
- **以下“尚未构建/未更新EXE/新增测试未运行”已过时**，以上交付结果取代对应状态，历史实现与诊断记录保留。

## 2026-10-30 自动分配内存开关（源码完成，未构建）

- 用户明确要求默认开启自动分配，每次启动游戏按此前曲线计算，关闭后才使用手动值。新增 AutomaticMemory，新岛品牌默认 true、普通 Prism 默认 false 且隐藏该控件；旧配置缺键直接采用默认 true，不删除或猜测迁移已有 MaxMemAlloc=4096。
- Java 设置内存组新增“自动分配内存”，全局及实例共用；实例通过既有 OverrideMemory 决定是否继承全局开关。自动模式显示计算结果并禁用最小/最大输入；关闭时恢复该窗口进入自动模式前的手动数值。保存自动模式不把预览值写入 MinMemAlloc/MaxMemAlloc，已有手动配置保留。
- MinecraftInstance::heapMemory 在需要启动参数时重新读取当前安装总内存（失败回退可见总内存），使用既定曲线；最小堆512MiB、最大堆沿(6,4)→(16,12)→(32,24)→(64,32)→(128,48)，之后封顶48GiB。硬件未知回退最大4096MiB；不按空闲内存比例计算，不缓存为永久配置。手动模式保留原版最小/最大颠倒时排序逻辑。
- 最终 -Xms/-Xmx、EnsureAvailableMemory 和 VerifyJavaInstall 32位提示统一调用该入口；设置页的测试Java/选32位Java检查也使用当前有效模式。保留原版空闲内存不足提示，不自动减小用户确认的曲线值。启动前读取是按需计算，非单次启动快照；不会持久化计算结果。
- 核查：git diff --check 通过；Qt uic 解析 UI 并生成仓库外头文件成功；审阅所有 MinMemAlloc/MaxMemAlloc 读写点，余下直接读取是手动设置/向导/导出推荐值，不用于游戏堆参数。已补 automaticAndManualMemory 回归用例（旧4096忽略、换机器曲线、关闭恢复手动、颠倒值、硬件未知），遵守“明确要求后才构建”约定，本轮未编译或执行新增测试、未运行GUI、未更新 EXE、未提交。
- **历史状态修正**：下文“仅改变默认值、没有自动开关”“询问是否清除旧4096”已被本节新方案替代；旧值保留给手动模式即可，无需清除。先前通过的测试属于开关加入前版本，不能视作本次改动通过运行验证。

## 2026-10-30 默认内存曲线与启动警告（源码及测试目标通过，EXE未更新）

- 用户确认图中坐标为（电脑总内存GiB，游戏最大堆GiB），执行默认策略：0–6按2/3；连接(6,4)、(16,12)、(32,24)、(64,32)、(128,48)，其后封顶48；整数MiB插值，不用浮点阈值。仅新岛品牌默认值改变，最小堆仍512MiB，普通Prism默认逻辑保留；这是按总内存的默认值，不是每次启动游戏按空闲内存80%分配。
- Windows优先GetPhysicallyInstalledSystemMemory读取安装容量，失败回退系统可见总内存；两种读取均未知时默认最大堆回退4096MiB。警告在安装内存<16GiB时显示；读取失败时用可见内存<15GiB容差，未知0不误判。Microsoft文档说明系统可见内存会扣除固件/设备保留，不是浮点精度问题：https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getphysicallyinstalledsystemmemory 。
- 首次显示新岛首页时复用CustomMessageBox::selectable，Warning图标、原有确认按钮，文案原样为“当前电脑内存较低（小于16GB），游戏可能不能正确运行。”；每进程最多一次，返回首页不重复，不阻止用户确认后继续。正常检测与测试强制条件共用同一弹窗。
- 程序旁niijima-local-test.json新增testWarning：按用户要求只判断key存在，true/false/null均强制弹同款窗口，删除key才关闭强制测试；不改变内存读取或分配。现有install-niijima JSON已添加true，原文件备份在独立工作目录prism-build/niijima-local-test-before-warning-20261030.json。完整格式见本地安装测试专题。
- 现有Roaming/NiijimaLauncher配置仍为MaxMemAlloc=4096，实例OverrideMemory=false，尚未删除或修改。已向用户询问是否清除，用户在追问保存机制，未批准。不能把已保存值认定为用户主动设置：Java向导确认及普通Java设置页应用会保存未改动的值，但当前自动下载Java开启时，createSetupWizard直接使javaRequired=false、不创建JavaWizardPage，不调用其validatePage。跳过向导不是自动confirm；本机4096最初写入来源仍未证实，不用通用保存路径冒充证据。没有自动迁移旧值。
- **2026-10-30 23:45 源码追查补充（替代上条仅列举可能入口的解释）**：全局设置通过 GenericPageProvider::getPages 一次构造所有页面，包含 JavaPage；JavaSettingsWidget 构造时 loadSettings 将有效默认值装入控件。PageDialog::accept 和 closeEvent 均经 handleClose → PageContainer::prepareToClose → saveAll 遍历所有页面 apply；JavaPage::apply → JavaSettingsWidget::saveSettings 在全局模式无条件写入 MinMemAlloc/MaxMemAlloc，不检查页面是否访问过或值是否改过。Setting::set 比较原始存储值，缺键与默认4096不相等，因此写入INI。故仅打开普通设置再点确定或窗口X，就能固化4096，无需打开Java页或手动改内存；取消按钮走reject，不是这一保存路径。现有 NiijimaLauncher-4.log 的14.502秒记录 Paged dialog close requested/approved、代理页应用及14.509秒保存全局配置，佐证这条批量保存路径已执行；日志未逐键记录，不能断言这是首次写入。自动Java跳过向导不走validatePage；SettingsObject::reload虽会set(get())，其现有调用在实例reloadSettings，内存OverrideSetting只写实例，不解释全局4096。当前配置不区分“默认值被顺带保存”和“用户主动选定”，不能仅凭数值4096自动迁移。此次仅核查并补充交接，未改配置/源码、未构建。
- **2026-10-30 原版来源核对**：git blame 确认 JavaSettingsWidget 全局内存无条件保存来自 1b4b36df33（2025-01-26，TheKodeToad），PageContainer::saveAll 批量应用机制可追溯至2018年MultiMC历史，当前循环形式由4463c21c98整理；这两文件无本地修改。因此固定内存配置及未改值随设置保存是继承行为，不是新岛跳过向导或此次曲线修改引入。将其称为缺少自动/手动区分更准确；没有证据称上游承认它是bug。
- 可见PowerShell仅构建LauncherTests目标及所需Launcher_logic，23:38完成；NiijimaLocalTest通过（含内存锚点/中点、单调性及上限、低内存边界、测试key存在性和原本本地下载校验，QtTest共6项含初始化/收尾）。git diff --check通过。未重建/替换启动器EXE、未实际展示新警告或游戏复测、未提交。运行目录仍为此前04:38 EXE。


## 2026-10-30 控制台按钮工业斜纹与自动内存能力核查
- **23:07预览修正**：用户指出上一版白字填充存在异常；此前“复看清晰”不足以作为验收。本次将字形本体从QPainterPath填充改回原有drawText，仅以路径绘制外围深色描边，独立预览编译及实际尺寸检查通过，白色笔画完整；三个状态仍为221×81。完整EXE未重建。具体路径填充异常的内部原因未进一步定位。
- 内存默认规则准确表述：总内存低于6144 MiB时默认最大堆为总内存/1.5（转换为整数MiB）；达到6144 MiB时固定4096 MiB。不是多个离散档位，也不是按可用内存计算；保存过的配置仍优先。

- 用户要求将游戏运行态的“打开控制台”按钮改为黄黑斜纹。HomePrimaryButton 已用 QPainter 绘制静态45度条纹及白字深色描边，沿用原点击、运行状态和固定尺寸；移除旧运行态纯色样式。此前04:39低饱和橙色记录为历史，本条仅取代源码样式，现有EXE仍是旧样式。
- 从当前源码提取按钮类进行独立Qt离屏预览，编译成功，三种状态均221×81；预览发现描边盖字后改为先描边再填字，复看清晰。预览位于独立工作目录 prism-build/button-stripes-preview.png。未重建完整启动器、未替换EXE、未进行实际GUI交互、未提交。
- 当前Prism的MinMemAlloc/MaxMemAlloc是可配置的固定值，实例可通过OverrideMemory覆盖全局；defaultMaxJvmMem只提供按总内存选择的默认值。EnsureAvailableMemory在启动时读取当前可用内存，仅提示或中止，不自动调整Xmx；MinecraftInstance直接将设置转成-Xms/-Xmx。当前没有按每次启动时空闲内存动态分配的开关。
- 用户提问PCL类似自动分配是否可行；现有HardwareInfo::availableRamMiB可复用。仅讨论可实现性，尚未确定比例、上限、默认启用范围或低内存策略，未实现也未新增待办；不把用户“大概80%”作为精确需求或已核实的PCL算法。


## 2026-10-30 22:52 测试实例 Java 堆耗尽（只读诊断）

- 实际运行实例在 C:/Users/AshBill/AppData/Roaming/NiijimaLauncher/instances/新岛铁道/minecraft，并非此前建议的独立 profile；本轮未更改配置或重启游戏。
- 启动器日志 NiijimaLauncher-0.log 确认参数为 -Xms512m、-Xmx4096m；全局 MaxMemAlloc=4096，实例 OverrideMemory=false。SysInfo.cpp 的 defaultMaxJvmMem 在物理内存至少 6 GiB 时默认返回 4096 MiB，未按本整合包需求提高。
- latest.log 在22:51:32进入世界，22:52:54首次报告 OutOfMemoryError: Java heap space；失败栈为 FTB Chunks 2001.3.8 的 MapRegionData.write 创建 BufferedImage / DataBufferInt。该栈只确认最终分配失败点，不能证明 FTB 内存泄漏或炼药锅功能致错；尚无堆转储或占用采样。22:52:59记录所有维度保存完成，不等同逐项存档验收。
- 诊断时系统约64 GiB内存、约53 GiB可用，但游戏Java进程已退出，此数值不是报错瞬间统计。优先判断4 GiB上限不足；建议此机器先以8 GiB上限复测相同场景，尚未应用或验证。未调整通用默认分配策略，未新增待办。


## 2026-10-30 休息前收尾

用户要求停止开发、保存工作并同步 Git。新发现的首页缺少手动最小化按钮尚未修复；实际启动／Java 安装／窗口切换验证和正式交付物收尾统一登记到[整合包总 todo](../../niijima-railways-modpack/agent/todo-list.md#新岛启动器收尾与下次测试2026-10-30)，本项目不另建平行待办。用户随后要求记录“整合包游戏默认最大化窗口启动”，也在总表中，尚未改配置。

最新可运行产物仍为下方04:39记录，今晚不再编译或启动游戏。下方“源码和文档尚未提交”描述其当时阶段；本次收尾将保存现有实现及镜像记录，提交结果以 Git 历史为准。

## 2026-10-30 04:39 首页启动状态、按钮配色及 Windows 窗口处理（已构建）

用户确定：点击启动后灰色“正在启动”；Java 游戏进程已运行后显示可点击的“打开控制台”，退出/失败/取消恢复“启动游戏”。“正在检查”仅指获取整合包发布索引；“正在安装”指包导入、下载、解压及运行文件准备，本地包/ZIP直链跳过索引查询。

- 首页只观察原生 LaunchController 生命周期、LaunchTask 的 PID/readyForLaunch，不改登录、下载或游戏启动顺序。Application 新增只读控制器查询与状态变化通知；首次 Microsoft 对话框期间也禁用按钮。
- “打开控制台”使用稍深的低饱和橙色；字号不变，只收紧五字文案间距。按钮固定按“启动游戏”的文本尺寸计算，已离屏渲染实际组件，三个状态均为221×81，颜色/对齐检查通过；渲染工具及图片留仓库外 prism-build/button-preview.*。
- Prism 原生 CloseAfterLaunch 只有“游戏窗口打开时隐藏启动器”开关，无自动最小化选项；原实现匹配 Setting user 日志并关闭启动器窗口，退出时打开 Prism 主窗口。本轮没有开启/覆盖该设置，而是在 Windows 新岛首页发起启动后每500ms观察对应游戏PID的可见、无owner、非tool顶层窗口；出现后仅一次请求 SetForegroundWindow，再最小化仍可见的首页。任务失败/取消/结束则停止等待，窗口关闭时Qt自动清理计时器。不做永久置顶，也不在“打开控制台”时触发此处理。
- [Windows 前台激活规则](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setforegroundwindow)可能拒绝激活请求；未使用模拟按键、挂接输入线程等强抢焦点手段。仅Windows首页适用，未新增前端设置或说明文字。已有隐藏设置仍保留原生行为。
- 用户中途要求暂不编译后已停止旧构建；本次在新增需求明确并获“做好后编译”授权后重新构建。修复一处回调参数遮蔽Qt成员的警告后，Release构建成功，CTest排除既有FileSystem后24/24通过。原测试目录install-niijima收集成功，EXE 41,572,352字节，生成于2026-10-30 04:38:33 +08:00。
- 验证边界：实际登录/Java准备期间按钮变化、Minecraft窗口激活与最小化、打开控制台及退出恢复尚待用户实测；编译/现有测试/离屏排版通过不等于这些GUI路径已经实测。本轮源码和文档尚未提交，不自动启动游戏。

## 2026-10-30 04:21 Java 自动安装资源已补齐

本地测试镜像与远端正式 ashbill 镜像均已增加用户指定的 Adoptium Temurin 17.0.20.1+1 Windows x64 JDK。新增官方 ZIP 和 Java17 两份元数据，更新根索引；官方 SHA-256、元数据链及公开 HTTP 地址均核对通过。没有改启动器源码/JavaPath，无需重新编译。用户重启启动器后再测自动 Java 安装及游戏启动，目前尚未实际验证这两步。详细文件、备份、来源和验证见《copyparty镜像准备》顶部。

下方“本地缺少 Java、未修复”的记录保留为诊断历史；其资源缺失状态已由本节替代。旧整镜像归档未重打，不能据旧归档部署后认为已含 Java。

## 2026-10-30 04:06 恢复后重新编译并更新运行目录

基于 `d9283a105`（源码恢复提交 `6f2320fd1`）重新完成 Release 编译，CTest 24/24 通过，沿用此前对 FileSystem 的排除。生成文件为 `build-ashbill/Release/niijima-launcher.exe`，包含恢复的首页 Microsoft 登录入口与解压 X/X 接线；未进行实际登录或完整安装测试。下载仍使用原并发队列，显示权重为 60/10/10/20。

首次收集因旧启动器占用 EXE 失败；用户关闭后，04:06 再次执行 CMake install 成功。`install-niijima/niijima-launcher.exe` 已更新为 41,566,720 字节、生成时间 2026-10-30 04:04:21 +08:00，与构建目录文件大小及时间一致。可直接从原运行目录启动测试；未重打安装器。此状态取代下方“尚未重新编译”，历史保留。

## 2026-10-30 03:57 恢复已提交（尚未重新编译）

已从原始工具补丁恢复首页直接 Microsoft 登录、解压 X/X 接线和首页60%初值，并与现有60/10/10/20进度代码/测试一起提交为 `6f2320fd1`。提交前后核对磁盘与提交内容一致；原下载并发流程保留。恢复的是02:38/03:18记录的原改动，不是重新凭记忆实现。03:42 EXE仍不含这次恢复，不能据此前24/24结果声称恢复后已编译。本轮未重新构建/启动GUI。下方“未恢复”均为历史，覆盖根因仍只是疑似编辑器旧缓冲回写。今后不默认在内置编辑器打开源码，规则已同步三处。

## 2026-10-30 03:53 覆盖追查补充

首页文件覆盖时间与 open_in_codex 打开另一文档的时间相差约64毫秒；首页在02:31已打开到编辑器，登录修改发生在02:38，符合旧编辑缓冲区后续回写覆盖的可能性。尚无直接写入进程证据，不认定为已证实根因。另一个整理提交的会话在提交前已发现缺失，没有提交首页源码。完整时间线见开屏专题顶部；未修复源码，恢复前先排除旧缓冲区再次回写。

## 2026-10-30 03:48 交付核对纠正：登录入口和解压显示缺失

用户指出直接登录仍未生效。现场检查 `NiijimaHomeWindow.cpp`：primaryAction 只有 `APPLICATION->launch(inst)`，此前的 hasMicrosoftAccount/MSALoginDialog 逻辑已不在；同文件解压 X/X 信号接线也缺失，局部初始值仍为 500。其他文件中的原单队列、60/10/10/20 权重和解压进度信号仍在。下段“本次编入首页直接添加 Microsoft 账户”及解压文案已交付的结论不成立；编译和 24/24 测试成功本身没有证明 UI 接线齐全。

运行进程路径为 install-niijima 下的 EXE，03:46:25 启动，晚于 03:42:24 新 EXE 生成，不能用旧进程解释。Git 全部分支历史搜索 hasMicrosoftAccount 未找到提交，近期 reflog 仅有三次提交，无相关 merge/reset/checkout；当前文件修改时间 03:21:55。证据支持工作区内容发生回退，但无法确认具体写入者或操作，不能归咎于 Git 合并或其他 agent。本轮仅核查并更正记录，未恢复代码或重新构建。

## 2026-10-30 03:42 构建完成：进度 60/10/10/20

用户要求的新显示比例已实现并构建：下载包60%、解压10%、Minecraft/Forge/全部依赖10%、assets20%。恢复后的原单队列并发下载不变。Release 编译、运行目录收集成功，CTest 排除既有 FileSystem 后 24/24 通过，含新增权重测试。`install-niijima/niijima-launcher.exe` 41,565,696 字节，修改时间 2026-10-30 03:42:24 +08:00；可继续使用原运行目录，测试配置保留。此前圆形图标、普通窗口名称、首页直接添加 Microsoft 账户等源码修改也随本次编入。没有启动 GUI 或实测完整安装，没有重打安装器。

下文“尚未构建”“60/10/20/10”是历史状态；最新实现/构建以本节及开屏专题顶部为准。现有分叉整合包仓库按用户要求留后合并，本轮未执行合并。

## 2026-10-30 修正状态：下载队列已恢复（未构建）

按用户要求撤掉进度条引入的 Forge 分类、串行队列和开关。LibrariesTask.h 与 HEAD 一致，cpp 只多一条进度转发；下载调度及原缓存/校验/重试/失败处理恢复原实现，显示权重为包 60%、解压 10%、全部库 20%、assets 10%。下段“尚未撤回”已过时，保留事件历史。测试源码同步调整，尚未编译或运行；现有 EXE 不变。各阶段大小依据现有 test0002 与镜像记于[开屏实现与验证](开屏实现与验证.md)顶部。

## 2026-10-30 进度修改越界事件（优先阅读）

用户明确指出：为展示 Minecraft/Forge 各 10% 而将原并发下载队列拆为两个串行队列，超出其意图。显示权重的批准不是改变下载调度的授权。详细原因、影响及教训见[开屏实现与验证](开屏实现与验证.md)顶部事件记录。源码中的拆分尚未撤回、尚未编译进 EXE；本次只记录，不视为已修复。下文“最终权重”记录保留为历史，不能据此认定该调度实现获得用户认可；后续 60/10/20/10 建议也尚未获采用。

## 2026-10-30 安装进度最终权重（尚未构建）

用户最终确定下载包 60%、解压 10%、Minecraft 10%、Forge 10%、assets 10%。源码已按 0/60/70/80/90/100 分段；解压显示“解压中 X/X”，下载显示速度和 ETA。正式自用包也为全量包，清单补下载不分配权重。安装预下载按 Forge 组件元数据拆分运行依赖，复用原下载与校验；普通启动队列不变。权重测试源码已补，静态检查通过，未编译或运行测试；ETA 仍仅覆盖活动请求，不是整个安装。详见开屏专题顶部。下文 50/50 和解压待决定描述已过时，当前 EXE 仍为 02:12 版本。

## 2026-10-30 窗口显示名与圆形图标（尚未构建）

普通窗口的显示名改为 Prism Launcher 加数字版本，去掉开发渠道后缀；仅改 Qt displayName，保留新岛 AppID/配置目录/自绘首页标题。SVG 图标背景裁为圆形，N 不缩放或移动；PNG 与原九尺寸 ICO 已用 Qt 导出并检查，EXE/安装器尚未重建。安装步骤耗时估算已补到开屏专题，明确不是实测且不用于直接确定权重。

## 2026-10-30 首页账号入口修订（尚未构建）

首页准备启动且没有 Microsoft 账号时，直接使用原 `MSALoginDialog::newAccount`，成功后保存并将新账号交给原启动流程；取消不启动。此前管理页来自原 LaunchController 路径。仅首页入口改变，已有账号的刷新、验证不改。代码与专题已更新，未构建或实际认证；与下段进度改动一同等待后续构建授权。

## 2026-10-30 进度与首页修订（尚未构建）

本轮删除安装成功说明，版本文字灰色缩小并底部对齐。新增独立部分进度通道，首页将包下载映射 0–50%、后续工作映射 50–100%，下载时显示当前活动请求的合计速度与 ETA；保留原 Prism 任务计数接口。长期规则已同步三份 AGENTS.md。解压有条目计数进度，主条停 50%；是否增加条内解压百分比已发选项，尚待回复。新增 Task 测试源码但未构建、未运行，当前 EXE 仍是 02:12 版本；详细接线及估算边界见[开屏实现与验证](开屏实现与验证.md)顶部。没有新打包、提交或推送。

## 2026-10-30 02:12 构建状态更新

用户授权后已重新完成 Release 编译、运行目录收集及 CTest（排除既有 FileSystem 弹窗测试，24/24 通过）。`install-niijima/niijima-launcher.exe` 已包含主按钮字距 11 px 和包根 `niijima-pack.json` 数值 `token: 114514` 的来源确认豁免；下文及专题中的“尚未构建”属于本次之前记录。`--version` 执行成功；尚未实际打开 GUI、导入或启动游戏。测试配置保留，仍指向 `build/pack-test.zip`，该包已成功递增到 `新岛铁道 v1.6.test0002`，包含 token，三处版本显示核对一致。下次成功打包为 test0003。沿用 `prism-local-test/启动本地测试.cmd` 可进入隔离测试 profile，直接运行 EXE 也可使用程序旁的本地测试配置；未制作新安装器、未提交或 push。

## 2026-10-30 C 盘本机构建与本地安装入口

- 本机仓库 `C:/mydata/Repositories/PrismLauncher`，`develop`；基线 `042e2a537`。新增程序旁的可选 `niijima-local-test.json`，指定本地 pack 与镜像；无配置保持正式流程。本轮源码未提交，EXE 显示的基线 Git hash 不包含未提交改动的信息。
- 已安装 VS 2022 Build Tools 17.14.43／MSVC 14.44、Windows SDK 26100、Qt 6.11.2 及仓库依赖。Release 编译及运行目录收集成功；`install-niijima/niijima-launcher.exe` 为 41,553,408 字节，`--version` 退出 0。未制作本轮 NSIS 安装器。
- CTest 排除已知 FileSystem 弹窗问题后 **24/24 通过**。首轮新增测试错误调用依赖 Application 单例的 Net::Request，在无该单例的测试进程中崩溃；改为实际 Qt 请求与 FileSink/ChecksumValidator 组合后通过。验证本地读取、错误哈希拒绝、本地缺文件失败。未运行 GUI、完整安装或游戏。
- 镜像已一次下载并解压到独立工作目录 `prism-local-test/mirror/niijima-mirror/`。本地 7 个 JSON 的 174 个资源 URL 已改为 ashbill 地址，三层 SHA-256 链核对通过，游戏资源未改。
- 程序旁已放测试配置，使用整合包 `build/pack-test.zip`（`1.6.test0001`）。启动入口为 `C:/mydata/Agents/Codex-CreateTrainResync/prism-local-test/启动本地测试.cmd`，使用同目录独立 `profile/`，不复用 PCL 实例。**正式发行前删除程序旁的测试 JSON。** 详见[本地安装测试](本地安装测试.md)。
- 自研共享终端运行副本在独立工作目录 `shared-terminal/`，已连接 `custom-sz-cn`；令牌不入库。只读确认 nginx 现有 `mc1.ashbill.com:25572/synology-root/` 映射，无需改 nginx。正式默认根已改为 `https://mc1.ashbill.com:25572/synology-root/minecraft/niijima-launcher/`。
- **远端状态：** 镜像根索引 HTTP 200；`packs/latest.json` HTTP 404，正式在线安装尚未部署。本轮服务器元数据替换已准备脚本并发出审批选项，尚未获得该项批准、未执行；远端元数据仍含 yake 地址。本地测试镜像已改址，不依赖该远端操作。
- **上述元数据待批准状态已过时（2026-10-30 01:39 更新）：** 用户已明确批准。共享终端已完成服务器 7 个 JSON 的备份、174 个 URL 改址及哈希链更新；线上 HTTP 逐文件核对全部通过，无 yake 下载地址残留。备份为 `/home/ashbill/niijima-mirror-meta-backup-20261030/`。未改 nginx 或游戏资源，正式包索引仍未由本轮上传。后续镜像发布也采用 ashbill 根，详见[镜像维护记录](copyparty镜像准备.md)顶部。
- 本机日志、首轮失败及最终通过记录在独立目录 `prism-build/`；计时记录在 `日志/任务耗时/20261030-prism-local-build.md`。SDK、下载缓存与测试数据保留复用。未 push。

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
