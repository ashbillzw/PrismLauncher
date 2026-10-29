# 开屏资源来源

2026-10-28 获取；不在运行时下载标题字体。

- `ZenMaruGothic-Bold.ttf`：[Google Fonts](https://github.com/google/fonts/tree/main/ofl/zenmarugothic)，字体授权见同目录 `OFL.txt`。
- `mmc_zh.qm`：[Prism 官方翻译服务](https://i18n.prismlauncher.org/0bfc1a6c74c387ddb5aabf9de977c861a347d141.class)，SHA1 `0bfc1a6c74c387ddb5aabf9de977c861a347d141`。简体中文代码为 `zh`，不是 `zh_CN`。
- `index_v2.json`：从官方翻译索引只保留 `zh`。启动时只复制缺失文件，之后仍由原翻译更新流程维护，不覆盖已下载的新版本。

- `home-background.png`：2026-10-29 用户确认应用的「新岛铁道背景-确定版.png」，2559×1439；来自用户提供的游戏截图，经传统算法调色，非 imagegen 重绘版。资源保留确定版原文件，模糊由开屏绘制，未修改桌面成品；2026-10-29 后续按用户要求统一减轻模糊，并去掉覆盖雾色及左右/底部遮罩（已随本轮构建生效）。处理记录见 `agent/handoff.md`。

- `NotoSansSC-VariableFont_wght.ttf`：2026-10-29 按用户指定内置 Noto Sans SC，直接复制本机已安装字体（Version 2.004-H2，wght 100–900），内部 family 为 `Noto Sans SC`。上游：[Google Fonts](https://github.com/google/fonts/tree/main/ofl/notosanssc)；附同目录 `NotoSansSC-OFL.txt`，授权原文来自上游 `OFL.txt`。中文控件默认使用本字体；日文控件显式使用 Zen Maru Gothic，不按共享汉字自动判断语言。

- `home-title.svg`：2026-10-29 用户明确要求采用的四字白色字标，基于内置 Zen Maru Gothic，仅将鉄右部竖笔出头去除；普通日文仍使用字体。来源/复现见 `README-home-title.md`。

## Zen Maru Gothic 字重（2026-10-29）

Google Fonts 的 [METADATA.pb](https://github.com/google/fonts/blob/main/ofl/zenmarugothic/METADATA.pb) 和[字体官方源库](https://github.com/googlefonts/zen-marugothic/tree/main/fonts/ttf) 当前仅提供五个静态字重：Light 300、Regular 400、Medium 500、Bold 700、Black 900，没有发布可变字体。本项目仅增加日文副标题需要的 Regular 400，并保留原有 Bold 700 和 `OFL.txt`；不在运行时下载字体。

- 新增文件：`ZenMaruGothic-Regular.ttf`。
- 下载来源：[Google Fonts 官方文件](https://raw.githubusercontent.com/google/fonts/main/ofl/zenmarugothic/ZenMaruGothic-Regular.ttf)；Google Fonts 元数据记录的源库提交为 `553c872b216d1290e2902a466edcdc9682f0df6a`。
- 字体内记录：family `Zen Maru Gothic`，style `Regular`，`Version 1.002`，`OS/2.usWeightClass = 400`，无 `fvar` 表（静态字体）。
- 文件长度：3,832,756 bytes。
- SHA-256：`a0c0b53543e0993ae2225e629c833f3d51495ad31720694ff112ce4ce11111ef`。
- 使用 fontTools 静态读取核对上述字段，并确认副标题 `==しゅっぱつしんこう！！==` 的全部字符均在 cmap 中。Regular 已随本轮构建内置；offscreen 检查确认为 Regular / OS2 weight400，没有运行启动器可见窗口。
- 已有 `ZenMaruGothic-Bold.ttf` 为 `Version 1.001`、`usWeightClass = 700`、无 `fvar`；标题 SVG 的生成基础仍保持 Bold。
