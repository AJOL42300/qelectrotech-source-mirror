"""Widen the realistic view of the EK1914 to the real housing proportions.

The housing of the EK1914 is 44 mm wide and 100 mm high (public data sheet),
like the EK1100. At the scale of the realistic views (3.3 px per mm) that is
about 145 px by 330 px, but the drawing was 88 px wide. The height is right
and is not touched: only the x coordinates are stretched, in two parts (the
RJ45 head, and the terminal block whose columns go from 10 to 20 px apart).
Circles keep their diameter, only their centres move. The pin numbers follow
their terminal. The script does nothing on a drawing that is already wide.

Usage: python widen_ek1914.py <elements dir>
"""
import os
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import num, write  # noqa: E402

SUBPATH = os.path.join("10_electric", "20_manufacturers_articles", "beckhoff",
                       "ek_couplers", "ek1914", "ek1914_realistic.elmt")
LEFT = -48.74        # left edge of the old body
HEAD_END = 14.0      # end of the RJ45 head, start of the terminal block
HEAD_END_NEW = 43.0
BLOCK_SCALE = 2.0    # 10 px between the columns become 20 px
SHIFT = -20.0        # centres the body on the hotspot
WIDTH = 160
HOTSPOT_X = 80
GRID = 5


def fx(x):
    if x <= HEAD_END:
        a = (HEAD_END_NEW - LEFT) / (HEAD_END - LEFT)
        return LEFT + (x - LEFT) * a + SHIFT
    return HEAD_END_NEW + (x - HEAD_END) * BLOCK_SCALE + SHIFT


def snap(v):
    return round(v / GRID) * GRID


def widen(root):
    desc = root.find("description")
    if float(root.get("width")) >= WIDTH:
        return False
    delta = {}
    for t in desc.findall("terminal"):
        old = float(t.get("x"))
        new = fx(old)
        if not t.get("name").isdigit():
            new = snap(new)
        delta[t.get("name")] = new - old
        t.set("x", num(new))
    for p in desc:
        tag = p.tag
        if tag == "line":
            for k in ("x1", "x2"):
                p.set(k, num(fx(float(p.get(k)))))
        elif tag in ("rect", "arc", "ellipse"):
            x0 = float(p.get("x"))
            x1 = x0 + float(p.get("width"))
            p.set("x", num(fx(x0)))
            p.set("width", num(fx(x1) - fx(x0)))
        elif tag == "circle":
            d = float(p.get("diameter"))
            p.set("x", num(fx(float(p.get("x")) + d / 2) - d / 2))
        elif tag == "polygon":
            for k in list(p.attrib):
                if k[0] == "x" and k[1:].isdigit():
                    p.set(k, num(fx(float(p.get(k)))))
        elif tag == "text":
            name = p.get("text")
            x = float(p.get("x"))
            if name in delta:
                p.set("x", num(x + delta[name]))
            elif name:
                p.set("x", num(fx(x)))
        elif tag == "dynamic_text":
            p.set("x", num(fx(float(p.get("x")))))
    root.set("width", str(WIDTH))
    root.set("hotspot_x", str(HOTSPOT_X))
    return True


def main(base):
    path = os.path.join(base, SUBPATH)
    tree = ET.parse(path)
    if widen(tree.getroot()):
        write(tree.getroot(), path)
        print("widened", path)
    else:
        print("already wide")


if __name__ == "__main__":
    main(sys.argv[1])
