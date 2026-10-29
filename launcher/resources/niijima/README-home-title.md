# 开屏四字字标

2026-10-29 用户按标注图明确要求采用；资源已接入开屏；构建结果与实际 GUI 验收范围见 agent/handoff.md。

- `home-title.svg`：白色、透明背景，四字均由现有 `ZenMaruGothic-Bold.ttf` 轮廓构成，无 `<text>`、嵌入位图或字体回退。
- 保留正常文字语义「新島鉄道」。只有「鉄」右部上横之上的竖笔出头被移除，形成用户要求的金旁加矢图形；未改变另外三个字、字宽、字间距或原字体文件。
- `viewBox="19 7 3956 979"`，宽高比约 **4.04086:1**。四周留 12 字体单位的微小安全边距；建议在 UI 里按比例显示，例如 404×100 或 444×110 逻辑像素，随 DPI 用矢量渲染。
- `generate_home_title.py` 从原字体可重复导出，需要开发机 Python 的 fontTools。脚本以原轮廓锚点断言避免误改不同版本字体；不写回字体。
- 图形与此前候选相同，仅收紧 viewBox 并更新描述。候选已通过 `title-glyph-comparison.png` 视觉核查；新旧 SVG 的四条路径和变换逐项相同。
- 字体来源与许可：项目内 Zen Maru Gothic Bold，随附 `OFL.txt`（SIL Open Font License）；上游 https://github.com/google/fonts/tree/main/ofl/zenmarugothic 。在项目中保留已有 OFL。

复现示例（Prism 仓库根目录，输出目录应预先存在）：

```powershell
python tools/niijima/generate_home_title.py --font launcher/resources/niijima/ZenMaruGothic-Bold.ttf --output launcher/resources/niijima/home-title.svg
```

原始候选先在仓库外准备；本文件与SVG现已入库接线。当前构建结果见交接记录，尚未由agent显示GUI验收。
