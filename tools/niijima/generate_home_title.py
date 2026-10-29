"""Export the approved Niijima wordmark as outlines; never modify the source font."""
from argparse import ArgumentParser
from copy import deepcopy
from pathlib import Path
from math import floor, ceil
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.transformPen import TransformPen


def export(font_path: Path, output: Path) -> None:
    font = TTFont(font_path)
    glyph_set = font.getGlyphSet()
    glyph = deepcopy(font['glyf']['uni9244'])
    coords, ends, flags = glyph.getCoordinates(font['glyf'])
    assert tuple(coords[42]) == (652, 651), 'Unexpected font outline; review the glyph before editing.'
    assert tuple(coords[50]) == (759, 651), 'Unexpected font outline; review the glyph before editing.'
    assert all(coords[i][1] >= 651 for i in range(43, 50))
    removed = set(range(43, 50))
    glyph.coordinates = type(coords)([xy for i, xy in enumerate(coords) if i not in removed])
    glyph.flags = bytearray(flag for i, flag in enumerate(flags) if i not in removed)
    glyph.endPtsOfContours = [end - 7 for end in ends]
    glyph.recalcBounds(font['glyf'])
    cmap = font.getBestCmap()
    paths = []
    bounds = BoundsPen(glyph_set)
    x = 0
    for char in '新島鉄道':
        name = cmap[ord(char)]
        path = SVGPathPen(glyph_set)
        bound = TransformPen(bounds, (1, 0, 0, -1, x, 880))
        if char == '鉄':
            glyph.draw(path, font['glyf'])
            glyph.draw(bound, font['glyf'])
        else:
            glyph_set[name].draw(path)
            glyph_set[name].draw(bound)
        paths.append(f'<path transform="translate({x} 0)" d="{path.getCommands()}"/>')
        x += font['hmtx'][name][0]
    x0, y0, x1, y1 = bounds.bounds
    margin = 12
    view = (floor(x0) - margin, floor(y0) - margin,
            ceil(x1) - floor(x0) + 2 * margin, ceil(y1) - floor(y0) + 2 * margin)
    viewbox = ' '.join(map(str, view))
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{viewbox}" role="img" aria-label="新島鉄道">'
           '<title>新島鉄道</title>'
           '<desc>Niijima Railways wordmark based on Zen Maru Gothic Bold. Only the upper vertical protrusion '
           'in the right component of 鉄 is removed; all other glyph outlines are unchanged. '
           'Approved for launcher use on 2026-10-29.</desc>'
           '<g transform="translate(0 880) scale(1 -1)" fill="#fff">' + ''.join(paths) + '</g></svg>\n')
    output.write_text(svg, encoding='utf-8')
    print(f'{output}: viewBox={viewbox}, aspect ratio={view[2] / view[3]:.6f}')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--font', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    export(args.font, args.output)
