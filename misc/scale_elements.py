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
    # "fixed" maps source coordinates to destination coordinates kept as is
    # (the power contacts of the cards must touch from one card to the next,
    # so they stay at the scaled distance of the datum).  When a terminal row
    # cannot be put on the grid without crossing a fixed coordinate, the
    # fixed ones are dropped (self.relaxed).
    def __init__(self, coords, factor, stages=()):
        self.s = factor
        self.relaxed = False
        for fixed in list(stages) + [{}]:
            if self._build(coords, factor, dict(fixed)):
                break
            self.relaxed = True

    def _build(self, coords, factor, fixed):
        clusters = []
        for c in sorted(set(coords) | set(fixed)):
            if clusters and abs(c - clusters[-1][-1]) < 0.75:
                clusters[-1].append(c)
            else:
                clusters.append([c])
        self.src = [sum(c) / len(c) for c in clusters]
        self.dst = []
        last = None
        for c, t in zip(clusters, self.src):
            if not any(v in coords for v in c) and any(v in fixed for v in c):
                self.dst.append(fixed[[v for v in c if v in fixed][0]])
            elif last is None:
                self.dst.append(snap_pos(factor * t))
                last = len(self.dst) - 1
            else:
                d = snap_delta(factor * (t - self.src[last]))
                self.dst.append(self.dst[last] + max(10.0, d))
                last = len(self.dst) - 1
        return all(b > a for a, b in zip(self.dst, self.dst[1:]))

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


def transform(text, factor, text_factor, datum=False):
    head, sep, body = text.partition("<description>")
    body, sep2, tail = body.partition("</description>")
    xs, ys = [], []
    for m in TAG_RE.finditer(body):
        if m.group(1) == "terminal":
            a = parse_attrs(m.group(2))
            if "x" in a and "y" in a:
                xs.append(float(a["x"]))
                ys.append(float(a["y"]))
    contacts = {}
    for m in TAG_RE.finditer(body):
        if m.group(1) == "arc":
            a = parse_attrs(m.group(2))
            if (a.get("width") == "8" and a.get("start") == "90"
                    and a.get("angle") == "180"
                    and re.search(r"color:(red|blue|orange)",
                                  a.get("style", ""))):
                y = float(a["y"]) + float(a["height"]) / 2
                contacts[y] = factor * y
    # Cards of one family share their datum (insertion point) and the edges
    # of their housing, so that they still touch when placed side by side.
    edges = {"x": [], "y": []}
    if datum:
        for m in TAG_RE.finditer(body):
            if m.group(1) in ("line", "rect", "circle", "ellipse", "arc",
                              "polygon"):
                a = parse_attrs(m.group(2))
                for ax, k1 in (("x", "width"), ("y", "height")):
                    if m.group(1) == "line":
                        edges[ax] += [float(a[ax + "1"]), float(a[ax + "2"])]
                    elif m.group(1) == "polygon":
                        i = 1
                        while ("%s%d" % (ax, i)) in a:
                            edges[ax].append(float(a["%s%d" % (ax, i)]))
                            i += 1
                    else:
                        size = float(a["diameter" if m.group(1) == "circle"
                                       else k1])
                        edges[ax] += [float(a[ax]), float(a[ax]) + size]
    bbox = {}
    for ax in ("x", "y"):
        if edges[ax]:
            bbox[ax] = {v: factor * v for v in (min(edges[ax]),
                                                max(edges[ax]))}
    fx = Axis(xs, factor, [bbox["x"]] if "x" in bbox else [])
    full = dict(bbox.get("y", {}))
    full.update(contacts)
    fy = Axis(ys, factor, [s for s in (full, bbox.get("y", {})) if s])
    keep_datum = datum
    dm = re.search(r"<definition\b[^>]*>", head)
    d = parse_attrs(dm.group(0))
    w, h = float(d["width"]), float(d["height"])
    hx, hy = float(d["hotspot_x"]), float(d["hotspot_y"])
    if xs and not keep_datum:
        # The insertion point moves onto the terminal nearest to the centre
        # of the terminals: the views of one product (same terminals, other
        # frame) keep the same insertion point.
        cx, cy = sum(xs) / len(xs), sum(ys) / len(ys)
        tx, ty = min(sorted(zip(xs, ys)),
                     key=lambda p: (p[0] - cx) ** 2 + (p[1] - cy) ** 2)
        ox, oy = fx(tx), fy(ty)
        fx.dst = [v - ox for v in fx.dst]
        fy.dst = [v - oy for v in fy.dst]

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
    minx, maxx = fx(-hx), fx(w - hx)
    miny, maxy = fy(-hy), fy(h - hy)
    minx = math.floor(minx / 10.0) * 10
    miny = math.floor(miny / 10.0) * 10
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
        out = transform(norm, args.factor, args.text_factor,
                        '/beckhoff/' in '/' + rel and '/drives_' not in rel)
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
