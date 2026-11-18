#!/usr/bin/env python3
# Copyright 2026 QElectroTech contributors
# License: GNU GPL v2 or later, see the licence block of the repository.
"""Write the layout (front view, real dimensions) elements of the Beckhoff set.

QElectroTech cannot scale an element (only mirror it), so the layout elements
exist in one folder per scale. The scale is a number of pixels per millimetre
of the real product:

  layout/1to1   1 px = 1 mm
  layout/1to2   1 px = 2 mm   (the scale of the cabinet elements of the IDE)
  layout/1to5   1 px = 5 mm

Every element is a plain front view: a rectangle at the real width and height
of the product, the reference as text, and a DIN rail element (35 mm) for the
modules to sit on. All elements share the same origin: the top left corner of
the module; the rail element draws its band 32.5 mm below that origin, so a
module and a rail placed at the same point are aligned.

Dimensions (millimetres, width x height; the depth goes in the description):
  EL terminals   12 x 100 x 68   standard EtherCAT Terminal housing; the width
                                 was not checked card by card
  EK1100         44 x 100 x 68   public data sheet
  EK1914         44 x 100 x 68   same housing as the EK1100 (not checked)
  EK1322         24 x 100 x 68   width from the public data sheet; height and
                                 depth assumed like the other couplers
  CU1128        122 x 100 x 38   public data sheet
  EPP2338-1002   30 x 126 x 26.5 public data sheet (without connectors)

The drives are not covered: their housing sizes were not checked.

The label is the reference. It is written rotated inside the body when the
body is narrow, horizontally when it is wide enough, and below the body when
nothing fits, with a point size that follows the scale (never below 4 pt).

Usage: python gen_layout.py <library folder>
"""
import math
import os
import sys
import uuid
import xml.etree.ElementTree as ET

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
RAIL_Y = 32.5      # mm from the top of a 100 mm module to the rail band
RAIL_H = 35.0
SILHOUETTE_W, EPP_H = 30.0, 126.0   # mm, EPP2338-1002 housing
LIGHT = "line-style:normal;line-weight:thin;filling:lightgray;color:black"
RAIL_LENGTH = 250.0

SCALES = (("1to1", 1.0, "1:1"), ("1to2", 0.5, "1:2"), ("1to5", 0.2, "1:5"))

# folder, reference, width, height, depth (mm), designation
MODULES = (
    ("EL1004", 12, 100, 68, "4-channel digital input terminal"),
    ("EL1008", 12, 100, 68, "8-channel digital input terminal"),
    ("EL1904", 12, 100, 68, "TwinSAFE digital input terminal"),
    ("EL1918", 12, 100, 68, "TwinSAFE digital input terminal"),
    ("EL2004", 12, 100, 68, "4-channel digital output terminal"),
    ("EL2008", 12, 100, 68, "8-channel digital output terminal"),
    ("EL2522", 12, 100, 68, "pulse train output terminal"),
    ("EL2574", 12, 100, 68, "digital output terminal"),
    ("EL2652", 12, 100, 68, "relay output terminal"),
    ("EL2904", 12, 100, 68, "TwinSAFE digital output terminal"),
    ("EL2911", 12, 100, 68, "TwinSAFE potential group terminal"),
    ("EL5152", 12, 100, 68, "incremental encoder terminal"),
    ("EL6910", 12, 100, 68, "TwinSAFE logic terminal"),
    ("EL9100", 12, 100, 68, "potential feed-in terminal"),
    ("EL9410", 12, 100, 68, "power feed terminal with diagnostics"),
    ("EK1100", 44, 100, 68, "EtherCAT coupler"),
    ("EK1322", 24, 100, 68, "EtherCAT P junction"),
    ("EK1914", 44, 100, 68, "EtherCAT coupler with TwinSAFE"),
    ("CU1128", 122, 100, 38, "EtherCAT junction, 8 ports"),
    ("EPP2338-1002", 30, 126, 26.5, "EtherCAT P box, 8 digital channels"),
)

LANGS = ("en", "fr", "de")
DIR_NAMES = {
    "layout": ("Layout (real dimensions)", "Implantation (dimensions r\u00e9elles)",
               "Aufbau (Originalma\u00dfe)"),
}


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(("layout",) + parts))


def num(v):
    return ("%.3f" % v).rstrip("0").rstrip(".")


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def font(pt):
    return "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular" % pt


def label_parts(text, wpx, hpx):
    """Return the text part(s) of a label for a body of wpx x hpx pixels.

    QElectroTech anchors a text by its far corner: the text box extends
    towards the top for a horizontal text and towards the left for a text
    rotated by 270 degrees, hence the th (text height) terms.
    """
    n = len(text)
    pt = min(7, max(4, int(wpx * 0.55)))
    th = pt * 1.33
    length = n * pt * 0.75
    if wpx >= length + 4:
        return [((wpx - length) / 2, 3.0 + th, 0, pt)]
    if wpx >= 5.5 and length <= hpx - 4:
        return [((wpx + th) / 2, hpx - (hpx - length) / 2, 270, pt)]
    pt, th = 6, 6 * 1.33
    return [((wpx + th) / 2, hpx + 4 + n * pt * 0.75, 270, pt)]


def text_xml(text, x, y, rot, pt):
    return ('\t\t<text x="%s" y="%s" rotation="%d" color="#000000" '
            'font="%s" text="%s" />\n'
            % (num(x), num(y), rot, font(pt), esc(text)))


def names_xml(names):
    return "".join('\t\t<name lang="%s">%s</name>\n' % (lang, esc(text))
                   for lang, text in zip(LANGS, names))


def definition(key, width, height, names, infos, body, desc):
    head = ('<definition version="0.100.0" type="element" link_type="simple" '
            'width="%d" height="%d" hotspot_x="0" hotspot_y="0">\n'
            % (math.ceil(width) + 2, math.ceil(height) + 2))
    out = [head, '\t<uuid uuid="%s" />\n' % uid(*key), "\t<names>\n",
           names_xml(names), "\t</names>\n", "\t<elementInformations>\n"]
    for name, value in infos:
        out.append('\t\t<elementInformation name="%s" show="1">%s'
                   "</elementInformation>\n" % (name, esc(value)))
    out.append("\t</elementInformations>\n\t<informations>")
    out.append(esc(desc) + " License: see "
               "http://qelectrotech.org/wiki/doc/elements_license"
               "</informations>\n\t<description>\n")
    out.append(body)
    out.append("\t</description>\n</definition>\n")
    return "".join(out)


def epp_body(scale):
    """Silhouette of the EPP2338-1002 (ears with their M3 hole, white label
    holders, M12 and M8 sockets, LEDs), from the geometry of gen_epp.py.
    The details that are smaller than about 1.5 px are left out."""
    import gen_epp as epp
    bx, by, bw, bh = epp.BODY
    k = SILHOUETTE_W * scale / bw          # px per unit of gen_epp
    wpx, hpx = SILHOUETTE_W * scale, EPP_H * scale
    ky = hpx / bh
    d = epp.Drawing("layout")

    def X(x):
        return (x - bx) * k

    def Y(y):
        return (y - by) * ky

    d.polygon(epp.silhouette(0, 0, wpx, hpx, tuple(
        r * scale for r in epp.SILHOUETTE_MM[2])), LIGHT)
    if scale >= 0.5:
        for hx, hy in epp.hole_centres(0, 0, wpx, hpx, scale):
            d.circle(hx, hy, epp.HOLE_MM[1] * scale, epp.WHITE)
        for cy in epp.SOCKET_Y:
            d.circle(X(0), Y(cy), 2 * epp.SOCKET_R * k, epp.METAL)
            d.circle(X(0), Y(cy), (2 * epp.SOCKET_R - 8) * k, epp.WHITE)
            hx, hw, hh = epp.HOLDER
            for hy in (cy - 25, cy + 9):
                d.rect(X(hx), Y(hy), hw * k, hh * k, epp.WHITE)
        hx, hw, hh = epp.HOLDER
        d.rect(X(hx), Y(epp.SOCKET_Y[-1] + 33), hw * k, hh * k, epp.WHITE)
        for cx in epp.M8_X:
            d.circle(X(cx), Y(epp.M8_Y), 2 * epp.M8_R * k, epp.RED)
    if scale >= 1:
        for cx, cy in (epp.LA_LEDS + (epp.RUN_LED,) + epp.POWER_LEDS
                       + tuple((lx, cy) for cy in epp.SOCKET_Y
                               for lx in epp.CHANNEL_LED_X)):
            d.circle(X(cx), Y(cy), epp.LED_D * k, epp.LED)
    out = ""
    for p in d.parts:
        out += "\t\t" + ET.tostring(p, encoding="unicode") + "\n"
    return out


def module(ref, w, h, d, designation, scale, tag):
    wpx, hpx = w * scale, h * scale
    if ref == "EPP2338-1002":
        body = epp_body(scale)
        pt, th = 6, 6 * 1.33
        length = len(ref) * pt * 0.75
        body += text_xml(ref, (wpx + th) / 2, hpx + 4 + length, 270, pt)
        hpx += length + 4
    else:
        body = ('\t\t<rect x="0" y="0" width="%s" height="%s" rx="0" ry="0" '
                'style="%s" antialias="false" />\n'
                % (num(wpx), num(hpx), LIGHT))
        for x, y, rot, pt in label_parts(ref, wpx, hpx):
            body += text_xml(ref, x, y, rot, pt)
    size = "%s x %s x %s mm" % (num(w), num(h), num(d))
    names = ("%s layout %s (%s)" % (ref, tag, size),
             "%s implantation %s (%s)" % (ref, tag, size),
             "%s Aufbau %s (%s)" % (ref, tag, size))
    infos = (("manufacturer", "Beckhoff"), ("manufacturer_reference", ref),
             ("designation", designation + ", front view " + tag))
    desc = ("Front view of the %s at the scale %s (1 px = %s mm), %s "
            "(width x height x depth). Generic outline, no logo; check the "
            "dimensions on the data sheet of your exact version."
            % (ref, tag, num(1 / scale), size))
    if ref == "EPP2338-1002":
        desc += (" Silhouette traced by eye on a photo (ears with the two "
                 "M3 fixing holes, 3.5 mm, hole position assumed). Fixing "
                 "(verified, Beckhoff EPP2xxx documentation, Fixing): two "
                 "M3 bolts longer than 15 mm, holes not threaded, no "
                 "torque given.")
    return definition((ref, tag), wpx, hpx + 30, names, infos, body, desc)


def rail(scale, tag):
    wpx, hpx = RAIL_LENGTH * scale, RAIL_H * scale
    y = RAIL_Y * scale
    body = ('\t\t<rect x="0" y="%s" width="%s" height="%s" rx="0" ry="0" '
            'style="line-style:normal;line-weight:thin;filling:gray;'
            'color:black" antialias="false" />\n'
            % (num(y), num(wpx), num(hpx)))
    body += ('\t\t<line x1="0" y1="%s" x2="%s" y2="%s" length1="1.5" '
             'length2="1.5" end1="none" end2="none" '
             'style="line-style:dashed;line-weight:thin;filling:none;'
             'color:black" antialias="false" />\n'
             % (num(y + hpx / 2), num(wpx), num(y + hpx / 2)))
    size = "35 x 7.5 mm, %s mm long" % num(RAIL_LENGTH)
    names = ("DIN rail %s layout %s" % (size, tag),
             "Rail DIN %s implantation %s" % (size, tag),
             "Hutschiene %s Aufbau %s" % (size, tag))
    infos = (("designation", "DIN rail 35 mm (EN 60715), front view " + tag),)
    desc = ("DIN rail band drawn %s mm under the origin so that it lines up "
            "with the modules of the same scale placed at the same point."
            % num(RAIL_Y))
    return definition(("rail", tag), wpx, RAIL_Y * scale + hpx + 2,
                      names, infos, body, desc)


def directory(names):
    return ("<qet-directory>\n\t<names>\n"
            + "".join('\t\t<name lang="%s">%s</name>\n' % (lang, esc(text))
                      for lang, text in zip(LANGS, names))
            + "\t</names>\n</qet-directory>\n")


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main(base):
    root = os.path.join(base, "layout")
    write(os.path.join(root, "qet_directory"), directory(DIR_NAMES["layout"]))
    count = 0
    for folder, scale, tag in SCALES:
        sub = os.path.join(root, folder)
        write(os.path.join(sub, "qet_directory"), directory((
            "Scale %s (1 px = %s mm)" % (tag, num(1 / scale)),
            "\u00c9chelle %s (1 px = %s mm)" % (tag, num(1 / scale)),
            "Ma\u00dfstab %s (1 px = %s mm)" % (tag, num(1 / scale)))))
        write(os.path.join(sub, "din_rail.elmt"), rail(scale, tag))
        count += 1
        for ref, w, h, d, designation in MODULES:
            write(os.path.join(sub, ref.lower() + ".elmt"),
                  module(ref, w, h, d, designation, scale, tag))
            count += 1
    print("%d layout elements written" % count)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
