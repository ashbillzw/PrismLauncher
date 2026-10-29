# NiijimaLauncher

面向新岛铁道的 Prism Launcher 定制分支。修改日期：2026-10-29。

- 定制、维护与发布：**AshBill**。
- AI 开发协作：**GPT-6 Astra**。
- 本分支源码：<https://github.com/ashbillzw/PrismLauncher>。
- 原项目：**Prism Launcher** 及其贡献者，<https://github.com/PrismLauncher/PrismLauncher>。
- 同时保留 **PolyMC**、**MultiMC** 原作者及第三方依赖的署名、版权和许可通知；完整作者与致谢可在程序的“关于 → 作者”中查看。

本分支不是 Prism 官方发行版，也不表示获得上游作者的背书。

## 许可与源码

本启动器及其定制代码以 **GNU GPL version 3（GPL-3.0-only）** 分发。它不提供任何担保；完整条款见同目录 `LICENSE`，原作者及第三方通知见 `COPYING.md` 和 `licenses/` 中的许可证文件。GPT-6 Astra 是开发协作署名，不替代原作者或版权所有人的声明。

源码仓库地址为上方链接。对外发布的每个二进制版本应同时提供与该版本相对应的完整源码、所需构建脚本和依赖获取说明；发行说明应给出对应 commit/tag 或源码归档。仓库内的 `agent/Windows安装器打包.md` 记录本分支的 Windows 构建方法。

此说明中的仓库链接不表示未提交的本地修改已上传。公开发布前，维护者需要把对应提交 push 到该公开仓库，并确认源码可访问；仅有指向旧版上游或缺少定制修改的链接不等于提供了本版本源码。

## 运行

运行同目录的 `niijima-launcher.exe`。请保留配套 DLL、Qt 插件、jars 等文件，不要只复制一个 EXE。

主要定制内容为新岛铁道开屏、实例安装/启动入口、镜像配置、独立 Windows 身份和图标；Minecraft、Forge、模组及账号服务各自的条款仍分别适用。
