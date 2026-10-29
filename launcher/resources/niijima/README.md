# 开屏资源来源

2026-10-28 获取；不在运行时下载标题字体。

- `ZenMaruGothic-Bold.ttf`：[Google Fonts](https://github.com/google/fonts/tree/main/ofl/zenmarugothic)，字体授权见同目录 `OFL.txt`。
- `mmc_zh.qm`：[Prism 官方翻译服务](https://i18n.prismlauncher.org/0bfc1a6c74c387ddb5aabf9de977c861a347d141.class)，SHA1 `0bfc1a6c74c387ddb5aabf9de977c861a347d141`。简体中文代码为 `zh`，不是 `zh_CN`。
- `index_v2.json`：从官方翻译索引只保留 `zh`。启动时只复制缺失文件，之后仍由原翻译更新流程维护，不覆盖已下载的新版本。

- `home-background.png`：2026-10-29 用户确认应用的「新岛铁道背景-确定版.png」，2559×1439；来自用户提供的游戏截图，经传统算法调色，非 imagegen 重绘版。资源保留确定版原文件，磨砂模糊和半透明层由开屏绘制，未修改桌面成品。处理记录见 `agent/handoff.md`。

- `NotoSansSC-VariableFont_wght.ttf`：2026-10-29 按用户指定内置 Noto Sans SC，直接复制本机已安装字体（Version 2.004-H2，wght 100–900），内部 family 为 `Noto Sans SC`。上游：[Google Fonts](https://github.com/google/fonts/tree/main/ofl/notosanssc)；附同目录 `NotoSansSC-OFL.txt`，授权原文来自上游 `OFL.txt`。中文控件默认使用本字体；日文控件显式使用 Zen Maru Gothic，不按共享汉字自动判断语言。

- `home-title.svg`：2026-10-29 用户明确要求采用的四字白色字标，基于内置 Zen Maru Gothic，仅将鉄右部竖笔出头去除；普通日文仍使用字体。来源/复现见 `README-home-title.md`。
