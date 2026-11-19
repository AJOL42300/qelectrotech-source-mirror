#!/usr/bin/env python3
# Scale the schematic elements of extra-elements so they match the size of
# the conventional QElectroTech symbols.
#
# Geometry is scaled by --factor around the hotspot and every terminal is
# moved onto the 10 px grid (relative to the hotspot) with a minimum spacing
# of 10 px.  The other coordinates follow a piecewise linear map which sends
# each terminal to its snapped position, so the lead lines still reach their
# terminal.  Static texts are scaled with --text-factor.
#
# Implantation (layout, footprint, IDE) elements and elements without any
# drawn geometry (pin tables) are never touched.
#
# The manifest (scale_elements_manifest.txt) stores, for each scaled file, the
# hash of the scaled content.  A file whose hash is in the manifest is skipped,
# so the script can be run again after regenerating elements.
import argparse
import hashlib
import math
import os
import re
import sys

PRIMS = ("line", "rect", "circle", "ellipse", "arc", "polygon", "text",
         "dynamic_text", "terminal", "plc_table")
TAG_RE = re.compile(r"<(%s)\b([^>]*?)(/?)>" % "|".join(PRIMS), re.S)
ATTR_RE = re.compile(r'([\w:-]+)="([^"]*)"')
SKIP_DIR_PARTS = ("ide-enclosures", "implantation", "/layout/")
SKIP_NAME_RE = re.compile(r"_(layout|fp|fp_face|fp_cutout|table)\.elmt$")


def fmt(v):
    v = round(v, 2)
    if v == int(v):
        return str(int(v))
    return ("%.2f" % v).rstrip("0").rstrip(".")


def snap_pos(v):
    return math.floor(v / 10.0 + 0.5) * 10.0


def snap_delta(v):
    return math.floor(v / 10.0 + 0.5 - 1e-9) * 10.0


class Axis:
    def __init__(self, coords, factor):
        self.s = factor
        clusters = []
        for c in sorted(coords):
            if clusters and abs(c - clusters[-1][-1]) < 0.75:
                clusters[-1].append(c)
            else:
                clusters.append([c])
        self.src = [sum(c) / len(c) for c in clusters]
        self.dst = []
        for i, t in enumerate(self.src):
            if i == 0:
                self.dst.append(snap_pos(factor * t))
            else:
                d = snap_delta(factor * (t - self.src[i - 1]))
                self.dst.append(self.dst[-1] + max(10.0, d))

    def __call__(self, v):
        if not self.src:
            return self.s * v
        if v <= self.src[0]:
            return self.dst[0] + self.s * (v - self.src[0])
        if v >= self.src[-1]:
            return self.dst[-1] + self.s * (v - self.src[-1])
        for i in range(1, len(self.src)):
            if v <= self.src[i]:
                a, b = self.src[i - 1], self.src[i]
                ga, gb = self.dst[i - 1], self.dst[i]
                return ga + (gb - ga) * (v - a) / (b - a)
        return self.s * v


def parse_attrs(s):
    return {k: v for k, v in ATTR_RE.findall(s)}


def set_attr(s, key, value):
    pat = re.compile(r'(\b%s=")([^"]*)(")' % re.escape(key))
    return pat.sub(lambda m: m.group(1) + value + m.group(3), s, count=1)


def scale_font(font, tf):
    parts = font.split(",")
    try:
        size = float(parts[1])
    except (IndexError, ValueError):
        return font
    new = max(3.0, round(size * tf * 2) / 2.0)
    parts[1] = fmt(new)
    return ",".join(parts)


def is_scalable(text):
    if "<description>" not in text:
        return False
    body = text[text.index("<description>"):]
    geo = re.search(r"<(line|rect|circle|ellipse|arc|polygon|terminal)\b", body)
    return geo is not None


def transform(text, factor, text_factor):
    head, sep, body = text.partition("<description>")
    body, sep2, tail = body.partition("</description>")
    xs, ys = [], []
    for m in TAG_RE.finditer(body):
        if m.group(1) == "terminal":
            a = parse_attrs(m.group(2))
            if "x" in a and "y" in a:
                xs.append(float(a["x"]))
                ys.append(float(a["y"]))
    fx, fy = Axis(xs, factor), Axis(ys, factor)

    def repl(m):
        kind, attrs, close = m.group(1), m.group(2), m.group(3)
        a = parse_attrs(attrs)
        out = attrs

        def put(k, v):
            nonlocal out
            out = set_attr(out, k, fmt(v))

        if kind == "line":
            put("x1", fx(float(a["x1"])))
            put("y1", fy(float(a["y1"])))
            put("x2", fx(float(a["x2"])))
            put("y2", fy(float(a["y2"])))
        elif kind == "rect":
            x, y = float(a["x"]), float(a["y"])
            w, h = float(a["width"]), float(a["height"])
            nx, ny = fx(x), fy(y)
            put("x", nx)
            put("y", ny)
            put("width", fx(x + w) - nx)
            put("height", fy(y + h) - ny)
            for k in ("rx", "ry"):
                if k in a:
                    put(k, float(a[k]) * factor)
        elif kind == "circle":
            x, y, d = float(a["x"]), float(a["y"]), float(a["diameter"])
            nd = d * factor
            put("x", fx(x + d / 2) - nd / 2)
            put("y", fy(y + d / 2) - nd / 2)
            put("diameter", nd)
        elif kind in ("ellipse", "arc"):
            x, y = float(a["x"]), float(a["y"])
            w, h = float(a["width"]), float(a["height"])
            nw, nh = w * factor, h * factor
            put("x", fx(x + w / 2) - nw / 2)
            put("y", fy(y + h / 2) - nh / 2)
            put("width", nw)
            put("height", nh)
        elif kind == "polygon":
            i = 1
            while ("x%d" % i) in a:
                put("x%d" % i, fx(float(a["x%d" % i])))
                put("y%d" % i, fy(float(a["y%d" % i])))
                i += 1
        elif kind == "terminal" or kind == "plc_table":
            if "x" in a and "y" in a:
                put("x", fx(float(a["x"])))
                put("y", fy(float(a["y"])))
        elif kind == "text":
            if "x" in a:
                put("x", fx(float(a["x"])))
                put("y", fy(float(a["y"])))
                if "font" in a:
                    out = set_attr(out, "font", scale_font(a["font"], text_factor))
        elif kind == "dynamic_text":
            if "x" in a:
                put("x", fx(float(a["x"])))
                put("y", fy(float(a["y"])))
                if "text_width" in a and float(a["text_width"]) > 0:
                    put("text_width", float(a["text_width"]) * factor)
        return "<%s%s%s>" % (kind, out, close)

    body = TAG_RE.sub(repl, body)

    dm = re.search(r"<definition\b[^>]*>", head)
    d = parse_attrs(dm.group(0))
    w, h = float(d["width"]), float(d["height"])
    hx, hy = float(d["hotspot_x"]), float(d["hotspot_y"])
    minx, maxx = fx(-hx), fx(w - hx)
    miny, maxy = fy(-hy), fy(h - hy)
    minx, miny = math.floor(minx), math.floor(miny)
    nw = int(math.ceil((maxx - minx) / 10.0) * 10)
    nh = int(math.ceil((maxy - miny) / 10.0) * 10)
    tag = dm.group(0)
    for k, v in (("width", nw), ("height", nh),
                 ("hotspot_x", int(-minx)), ("hotspot_y", int(-miny))):
        tag = set_attr(tag, k, str(v))
    head = head[:dm.start()] + tag + head[dm.end():]
    return head + sep + body + sep2 + tail


def sha(text):
    return hashlib.sha1(text.encode("utf-8")).hexdigest()


def candidates(root):
    for dp, dn, fn in os.walk(root):
        rel = dp.replace("\\", "/") + "/"
        if any(p in rel for p in SKIP_DIR_PARTS):
            continue
        if re.search(r"/beckhoff/layout/", rel):
            continue
        for f in sorted(fn):
            if f.endswith(".elmt") and not SKIP_NAME_RE.search(f):
                yield os.path.join(dp, f)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("root", help="extra-elements directory")
    ap.add_argument("--factor", type=float, default=0.5)
    ap.add_argument("--text-factor", type=float, default=0.7)
    ap.add_argument("--manifest", default=os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "scale_elements_manifest.txt"))
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    done = {}
    if os.path.exists(args.manifest):
        for line in open(args.manifest, encoding="utf-8"):
            line = line.rstrip("\n")
            if line:
                h, p = line.split(" ", 1)
                done[p] = h
    n_scaled = n_skipped = n_known = 0
    for path in candidates(args.root):
        rel = os.path.relpath(path, args.root).replace("\\", "/")
        raw = open(path, "rb").read()
        text = raw.decode("utf-8")
        norm = text.replace("\r\n", "\n")
        if done.get(rel) == sha(norm):
            n_known += 1
            continue
        if not is_scalable(norm):
            n_skipped += 1
            continue
        out = transform(norm, args.factor, args.text_factor)
        n_scaled += 1
        if not args.dry_run:
            open(path, "wb").write(out.encode("utf-8"))
            done[rel] = sha(out)
    if not args.dry_run:
        with open(args.manifest, "w", encoding="utf-8", newline="\n") as f:
            for p in sorted(done):
                f.write("%s %s\n" % (done[p], p))
    print("scaled=%d already=%d skipped(no geometry)=%d" % (
        n_scaled, n_known, n_skipped))


if __name__ == "__main__":
    sys.exit(main())
