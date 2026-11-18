#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
"""Write the Schneider Electric library of extra-elements.

Circuit breakers, RCDs, disconnectors (Acti9, ComPacT NSX / INS), TeSys D
contactors and the accessories (auxiliary contacts, LADN blocks, motor
mechanism...).  For each product, in its own folder:
  <stem>.elmt              IEC schematic symbol (when the product has one)
  <stem>_realistic.elmt    front view, vector drawing
  <stem>_accessories.elmt  front view with the accessories fitted (NSX250)

Only facts are used: references, number of poles, rating, width in
modules, outside dimensions, terminal numbers.  Everything else (the
position of the handle, the labels, the colours) is a generic layout,
redrawn, not copied from a manufacturer drawing; no logo and no manufacturer
text is drawn.  The facts come from the public product pages of the
manufacturer, see docs/schneider.md, where what is NOT verified is listed.
The colours are the named colours of the element editor nearest to the
public product photos (green accent, light and dark greys).

Scale.  Modular devices (Acti9) and TeSys D: 1 module of 9 mm = 30 px, so a
pole of 18 mm is 60 px (3.33 px/mm).  Compact NSX / INS: the pole pitch of
35 mm is 120 px (3.43 px/mm).  The first top terminal is the origin of every
element (hotspot on a connection point) and every connection point and every
frame is on the 10 px grid.  The accessories are drawn at their place on an
NSX250 3P, with the same origin, so that they stack on it.

Usage: python gen_schneider.py <extra-elements>/10_electric/20_manufacturers_articles/schneider
"""
import math
import os
import shutil
import sys
import uuid
from xml.sax.saxutils import quoteattr

NS = uuid.UUID("4d1c7a52-93b7-4f0e-8e0a-2c6f5b9a1d30")
MANUFACTURER = "Schneider Electric"
LICENSE = "License: see http://qelectrotech.org/wiki/doc/elements_license"
INFO = ("Author: QElectroTech contributors\nGeneric layout redrawn from the "
        "public data of the product (reference, poles, rating, "
        "dimensions), not copied from a manufacturer drawing. " + LICENSE)
FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
FONT_BOLD = "Roboto Condensed,%d,-1,5,75,0,0,0,0,0,Bold"

DIRECTORIES = {
    "": ("Schneider Electric", "Schneider Electric", "Schneider Electric"),
    "acti9_ic60": ("Acti9 iC60 miniature circuit breakers",
                   "Disjoncteurs modulaires Acti9 iC60",
                   "Acti9 iC60 Leitungsschutzschalter"),
    "acti9_rcd": ("Acti9 residual current devices",
                  "Dispositifs diff\u00e9rentiels Acti9",
                  "Acti9 Fehlerstromschutzeinrichtungen"),
    "acti9_isolators": ("Acti9 switch-disconnectors and fuse-disconnectors",
                        "Interrupteurs-sectionneurs et sectionneurs "
                        "fusibles Acti9",
                        "Acti9 Lasttrennschalter und Sicherungstrenner"),
    "compact_nsx": ("ComPacT NSX moulded case circuit breakers",
                    "Disjoncteurs bo\u00eetier moul\u00e9 ComPacT NSX",
                    "ComPacT NSX Kompaktleistungsschalter"),
    "compact_ins": ("ComPacT INS switch-disconnectors",
                    "Interrupteurs-sectionneurs ComPacT INS",
                    "ComPacT INS Lasttrennschalter"),
    "accessories": ("ComPacT NSX and Acti9 accessories",
                    "Accessoires ComPacT NSX et Acti9",
                    "ComPacT NSX und Acti9 Zubeh\u00f6r"),
    "tesys_d": ("TeSys D contactors",
                "Contacteurs TeSys D",
                "TeSys D Sch\u00fctze"),
    "tesys_d_accessories": ("TeSys D auxiliary contact blocks",
                            "Blocs de contacts auxiliaires TeSys D",
                            "TeSys D Hilfsschalterbl\u00f6cke"),
}
# Folders whose products are accessories of another product
ACCESSORY_FOLDERS = {"accessories", "tesys_d_accessories"}

# Colours.  An element can only use the named colours of the element
# editor, so the Schneider green (about #3DCD58 on the public photos) is the
# nearest one, HTMLGreenLimeGreen (#32CD32); the greys are the named greys.
# Status: ASSUMED, chosen from the public product photos, see
# docs/schneider.md.
GREEN = "HTMLGreenLimeGreen"
WHITE = "white"
BLACK = "black"
LIGHT = "HTMLGrayGainsboro"      # light grey housings, labels
SILVER = "HTMLGraySilver"        # screws, metal
MID = "HTMLGrayDarkGray"         # front plates
DARK = "HTMLGrayDimGray"         # dark housings (ComPacT, TeSys)
ON_RED = "red"
WARN = "yellow"


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def esc(text):
    return quoteattr(text)[1:-1]


def up10(v):
    return int(math.ceil(v / 10.0 - 1e-9) * 10)


def down10(v):
    return int(math.floor(v / 10.0 + 1e-9) * 10)


def num(v):
    return ("%.2f" % v).rstrip("0").rstrip(".")


# ---------------------------------------------------------------- drawing

def style(fill="none", color="black", weight="thin", dashed=False):
    return "line-style:%s;line-weight:%s;filling:%s;color:%s" % (
        "dashed" if dashed else "normal", weight, fill, color)


class Sheet:
    """A drawing: parts in painting order, then terminals."""

    def __init__(self):
        self.parts = []
        self.terminals = []
        self.bbox = None

    def grow(self, x0, y0, x1, y1):
        b = self.bbox
        self.bbox = ((x0, y0, x1, y1) if b is None else
                     (min(b[0], x0), min(b[1], y0), max(b[2], x1),
                      max(b[3], y1)))

    def rect(self, x, y, w, h, fill="none", weight="thin", frame=False):
        self.parts.append(
            '<rect x="%s" y="%s" width="%s" height="%s" antialias="false" '
            'style="%s" rx="0" ry="0" />' % (
                num(x), num(y), num(w), num(h), style(fill, "black", weight)))
        if frame:
            self.grow(x, y, x + w, y + h)

    def line(self, x1, y1, x2, y2, weight="thin", dashed=False):
        self.parts.append(
            '<line end1="none" end2="none" y1="%s" antialias="false" '
            'y2="%s" style="%s" x1="%s" length1="1.5" x2="%s" '
            'length2="1.5" />' % (num(y1), num(y2),
                                  style("none", "black", weight, dashed),
                                  num(x1), num(x2)))

    def circle(self, cx, cy, d, fill="none", weight="thin"):
        self.parts.append(
            '<circle x="%s" y="%s" antialias="false" style="%s" '
            'diameter="%s" />' % (num(cx - d / 2.0), num(cy - d / 2.0),
                                  style(fill, "black", weight), num(d)))

    def text(self, cx, cy, s, size=6, bold=False, color="#000000"):
        """Text centred on (cx, cy)."""
        width = len(s) * size * (0.62 if bold else 0.55)
        font = (FONT_BOLD if bold else FONT) % size
        self.parts.append(
            '<text x="%s" y="%s" rotation="0" color="%s" font="%s" '
            'text="%s" />' % (num(cx - width / 2.0),
                              num(cy + 0.55 * size), color, font, esc(s)))

    def terminal(self, name, x, y, orientation):
        self.terminals.append((name, x, y, orientation))


def screw(sh, x, y, d=16):
    sh.circle(x, y, d, SILVER, "normal")
    sh.circle(x, y, d - 6, "HTMLGrayLightSlateGray")
    sh.line(x - d / 2.0 + 3, y, x + d / 2.0 - 3, y, "normal")


def mark(sh, name, x, y, size=5, side="r", color="#000000"):
    """The number of a terminal, written beside the connection point: at the
    right (r) or the left (l) of (x, y), or centred on it (c)."""
    width = len(name) * size * 0.55
    cx = {"r": x + 4 + width / 2.0, "l": x - 4 - width / 2.0,
          "c": x}[side]
    sh.text(cx, y, name, size, False, color)


# ---------------------------------------------------------------- products

class Product:
    def __init__(self, folder, stem, reference, designation, description,
                 names, poles, labels, kind, **kw):
        self.folder = folder
        self.stem = stem
        self.reference = reference
        self.designation = designation
        self.description = description
        self.names = names              # (en, fr)
        self.poles = poles
        self.labels = labels            # terminal names, (top, bottom)
        self.kind = kind
        self.accessories = False
        self.__dict__.update(kw)


def pole_labels(n, neutral_left=False):
    labels = []
    if neutral_left:
        labels.append(("N1", "N2"))
        n -= 1
    for k in range(n):
        labels.append((str(2 * k + 1), str(2 * k + 2)))
    return labels


PRODUCTS = []


def add(*a, **kw):
    PRODUCTS.append(Product(*a, **kw))


# Acti9 iC60N, C curve, 10 kA (IEC/EN 60947-2): 1P A9F74110 10 A,
# 2P A9F74202 2 A (the one of examples/industrial.qet), 3P A9F74310 and
# 4P A9F74410
for n, ref, rating in ((1, "A9F74110", 10), (2, "A9F74202", 2),
                       (3, "A9F74310", 10), (4, "A9F74410", 10)):
    add("acti9_ic60", "ic60n_%dp" % n, ref,
        "iC60N miniature circuit breaker, %dP, %d A, C curve" % (n, rating),
        "Miniature circuit breaker iC60N, %d pole%s, %d A, C curve, "
        "thermal-magnetic, 10 kA (IEC/EN 60947-2). Width %d x 9 mm "
        "(%d mm), height 85 mm. One terminal per pole, top and bottom "
        "(1-2, 3-4...)." % (n, "s" if n > 1 else "", rating, 2 * n, 18 * n),
        ("iC60N circuit breaker %dP %d A C" % (n, rating),
         "Disjoncteur iC60N %dP %d A C" % (n, rating)),
        n, pole_labels(n), "breaker", xs=[60 * k for k in range(n)],
        body=(-30, 60 * n - 30), height=280, rating="C%d" % rating,
        model="iC60N", wmm=18 * n, hmm=85, dmm=None)

# Residual current devices
add("acti9_rcd", "idpna_vigi_1pn", "A9D34616",
    "iDPNa Vigi residual current circuit breaker, 1P+N, 16 A, C curve, "
    "30 mA, type AC",
    "RCBO iDPNa Vigi, 1P+N, 16 A, C curve, 4.5 kA, type AC, 30 mA. "
    "Width 2 x 9 mm (18 mm). Terminals: phase 1-2, neutral N1-N2. "
    "Replaced by iCV40 in the catalogue.",
    ("iDPNa Vigi RCBO 1P+N 16 A C 30 mA AC",
     "Disjoncteur diff\u00e9rentiel iDPNa Vigi 1P+N 16 A C 30 mA AC"),
    2, [("1", "2"), ("N1", "N2")], "rcbo", xs=[0, 30], body=(-15, 45),
    height=280, rating="C16", sens="30mA", rtype="AC", model="iDPNa Vigi",
    wmm=18, hmm=85, dmm=None)
add("acti9_rcd", "vigi_ic60_3p", "A9V11325",
    "Vigi iC60 add-on residual current device, 3P, 25 A, 30 mA, type AC",
    "Add-on residual current device Vigi iC60, 3P, 25 A, 30 mA, type AC, "
    "instantaneous, 400-415 V AC, for iC60. Width 6 x 9 mm (54 mm), height "
    "91 mm, depth 73.5 mm. Discontinued 23 Sept 2025, not replaced. "
    "Mounted next to an iC60: its own terminals are drawn.",
    ("Vigi iC60 add-on RCD 3P 25 A 30 mA AC",
     "Bloc diff\u00e9rentiel Vigi iC60 3P 25 A 30 mA AC"),
    3, pole_labels(3), "rcd", xs=[0, 60, 120], body=(-30, 150),
    height=300, rating="25A", sens="30mA", rtype="AC", model="Vigi iC60",
    wmm=54, hmm=91, dmm=73.5)
for n, ref, amp, verified in ((2, "A9R41225", 25, False),
                              (4, "A9R41440", 40, True)):
    add("acti9_rcd", "iid_%dp" % n, ref,
        "iID residual current circuit breaker, %dP, %d A, 30 mA, type AC" %
        (n, amp),
        "RCCB iID, %dP, %d A, 30 mA, type AC, instantaneous. Neutral on "
        "the left (N1-N2). %s" % (
            n, amp, "Width 8 x 9 mm (72 mm), height 85-91 mm, depth "
            "69-73.5 mm." if verified else
            "Width 4 x 9 mm (36 mm) ASSUMED from the pole pitch."),
        ("iID RCCB %dP %d A 30 mA AC" % (n, amp),
         "Interrupteur diff\u00e9rentiel iID %dP %d A 30 mA AC" % (n, amp)),
        n, pole_labels(n, neutral_left=True), "rcd",
        xs=[60 * k for k in range(n)], body=(-30, 60 * n - 30), height=300,
        rating="%dA" % amp, sens="30mA", rtype="AC", model="iID",
        wmm=18 * n, hmm=91, dmm=73.5)

# Switch-disconnector and fuse-disconnector
add("acti9_isolators", "isw_3p_40a", "A9S65340",
    "iSW switch-disconnector, 3P, 40 A, 415 V",
    "Switch-disconnector iSW, 3P, 40 A at 415 V AC (AC-22A), EN/IEC "
    "60947-3, 6 x 9 mm (54 mm) wide, 85 mm high, 73 mm deep. Can be fitted "
    "with an OF auxiliary contact.",
    ("iSW switch-disconnector 3P 40 A",
     "Interrupteur-sectionneur iSW 3P 40 A"),
    3, pole_labels(3), "switch", xs=[0, 60, 120], body=(-30, 150),
    height=280, rating="40A", model="iSW", wmm=54, hmm=85, dmm=73)
add("acti9_isolators", "sti_2p_25a", "A9N15651",
    "STI fuse-disconnector, 2P, 25 A, for 10.3 x 38 fuses",
    "Fuse-disconnector STI, 2P, 25 A, for 10.3 x 38 mm fuses, 4 x 9 mm "
    "(36 mm) wide, 81 mm high, 75 mm deep.",
    ("STI fuse-disconnector 2P 25 A", "Sectionneur fusible STI 2P 25 A"),
    2, pole_labels(2), "fuse", xs=[0, 60], body=(-30, 90), height=270,
    rating="25A", model="STI", wmm=36, hmm=81, dmm=75)

# ComPacT NSX and INS.  3P: 105 x 161 x 86 mm, pole pitch 35 mm = 120 px
NSX_TOP, NSX_BOTTOM = -60, 490


def nsx(stem, ref, designation, description, names, poles, **kw):
    add("compact_nsx", stem, ref, designation, description, names, poles,
        pole_labels(poles), "mccb", xs=[120 * k for k in range(poles)],
        body=(-60, 120 * (poles - 1) + 60), height=NSX_BOTTOM - NSX_TOP,
        wmm=105, hmm=161, dmm=86, **kw)


nsx("nsx100b_3p_tmd100", "LV429540",
    "ComPacT NSX100B circuit breaker, 3P, 100 A, TMD, 25 kA",
    "Circuit breaker ComPacT NSX100B, 3P fixed, 25 kA at 415 V, thermal-"
    "magnetic trip unit TMD 100 A. 105 x 161 x 86 mm, pole pitch 35 mm.",
    ("ComPacT NSX100B 3P 100 A TMD", "ComPacT NSX100B 3P 100 A TMD"), 3,
    model="NSX100B", rating="100 A", trip="TMD100")
nsx("nsx250n_3p_frame", "C25N3",
    "ComPacT NSX250N basic frame, 3P, 250 A, 50 kA, without trip unit",
    "Basic frame ComPacT NSX250N, 3P, 250 A, 50 kA at 415 V, without trip "
    "unit. 105 x 161 x 86 mm.",
    ("ComPacT NSX250N 3P frame, no trip unit",
     "ComPacT NSX250N 3P ch\u00e2ssis sans d\u00e9clencheur"), 3,
    model="NSX250N", rating="250 A", trip=None)
nsx("nsx250n_3p_tm250", "C25N3TM250C",
    "ComPacT NSX250N circuit breaker, 3P, 250 A, thermal-magnetic TM250",
    "Circuit breaker ComPacT NSX250N, 3P, 250 A, 50 kA at 415 V, "
    "thermal-magnetic trip unit TM250. 105 x 161 x 86 mm. The "
    "_accessories appearance shows it with the motor mechanism MT250 "
    "LV431541 (220/240 V AC), the auxiliary contact 29450 (OF/SD/SDE), "
    "the undervoltage release MN LV429410 (24 V DC) and the long terminal "
    "shields LV429517.",
    ("ComPacT NSX250N 3P 250 A TM250", "ComPacT NSX250N 3P 250 A TM250"),
    3, model="NSX250N", rating="250 A", trip="TM250", accessories=True)
for n, ref in ((3, "31106"), (4, "31107")):
    add("compact_ins", "ins250_%dp" % n, ref,
        "ComPacT INS250 switch-disconnector, %dP, 250 A, black rotary "
        "handle" % n,
        "Non-automatic switch-disconnector ComPacT INS250, %d poles, 250 A "
        "at 415 V AC (AC-22A, AC-23A), 690 V AC / 250 V DC, standard "
        "version with black rotary handle, isolation with positive contact "
        "indication. Outside dimensions ASSUMED equal to the NSX250 "
        "(%d x 161 x 86 mm)." % (n, 105 if n == 3 else 140),
        ("ComPacT INS250 %dP 250 A switch-disconnector" % n,
         "Interrupteur-sectionneur ComPacT INS250 %dP 250 A" % n),
        n, pole_labels(n), "ins", xs=[120 * k for k in range(n)],
        body=(-60, 120 * (n - 1) + 60), height=NSX_BOTTOM - NSX_TOP,
        model="INS250", rating="250 A", wmm=105 if n == 3 else 140,
        hmm=161, dmm=86)

# Accessories, drawn at their place on the front of an NSX250 3P: same
# origin as the breaker, so that they stack on it.
ACC_BODY = (-60, 300)
add("accessories", "iof_a9a26924", "A9A26924",
    "iOF auxiliary contact, Acti9, 1 changeover, AC/DC",
    "Acti9 iOF open/close auxiliary contact (also fault indication), "
    "1 changeover (1 NO + 1 NC), 6 A at 240 V AC, 9 mm wide, for iC60, "
    "iID, iDPN Vigi and iSW 40-125 (public compatibility list). "
    "Terminals C, NC, NO ASSUMED.",
    ("iOF auxiliary contact A9A26924",
     "Contact auxiliaire iOF A9A26924"), 0, [],
    "iof", body=(-15, 15))
add("accessories", "mt250_220_240vac", "LV431541",
    "Motor mechanism module MT250, ComPacT NSX250, 220/240 V AC",
    "Standard motor mechanism module MT250 for ComPacT NSX250, control "
    "voltage 220-240 V AC 50/60 Hz (208-277 V AC 60 Hz), consumption below "
    "500 VA, automatic spring charging, padlocking still possible, supplied "
    "with an SDE adapter. Motor terminals M1-M2 ASSUMED.",
    ("MT250 motor mechanism 220/240 V AC",
     "Commande motoris\u00e9e MT250 220/240 V CA"), 0, [],
    "motor", body=ACC_BODY, supply="220/240 V AC")
add("accessories", "mt250_24_30vdc", "LV431543",
    "Motor mechanism module MT250, ComPacT NSX250, 24/30 V DC",
    "Standard motor mechanism module MT250 for ComPacT NSX250, control "
    "voltage 24/30 V DC, consumption below 500 W, supplied with an SDE "
    "adapter. Motor terminals M1-M2 ASSUMED.",
    ("MT250 motor mechanism 24/30 V DC",
     "Commande motoris\u00e9e MT250 24/30 V CC"), 0, [],
    "motor", body=ACC_BODY, supply="24/30 V DC")
add("accessories", "mn_24vdc", "LV429410",
    "MN undervoltage release, ComPacT NSX, 24 V DC, spring terminals",
    "Instantaneous undervoltage release MN for ComPacT NSX, 24 V DC, "
    "spring terminals. Coil terminals D1-D2 ASSUMED.",
    ("MN undervoltage release 24 V DC",
     "D\u00e9clencheur MN \u00e0 minimum de tension 24 V CC"), 0, [],
    "mn", body=ACC_BODY, supply="24 V DC")
add("accessories", "aux_of_sd_29450", "29450",
    "Auxiliary contact OF/SD/SDE/SDV, 1 changeover, spring terminals",
    "Standard auxiliary contact for ComPacT NSX, NS, PowerPact and "
    "EasyPact CVS: ON/OFF (OF), trip (SD), fault trip (SDE) and earth-"
    "fault trip (SDV) indication depending on its slot. One changeover "
    "contact (1 NO + 1 NC on a common point), 6 A, spring terminals. "
    "Terminals C, NC, NO.",
    ("OF/SD/SDE/SDV auxiliary contact 29450",
     "Contact auxiliaire OF/SD/SDE/SDV 29450"), 0, [],
    "aux", body=ACC_BODY)
add("accessories", "rotary_handle_lv429337", "LV429337",
    "Direct rotary handle, ComPacT NSX100/160/250, black",
    "Direct rotary handle for ComPacT NSX100/160/250, black handle, "
    "padlockable in the OFF position, degree of protection IP40 (IK07). "
    "Mechanical accessory, no terminal.",
    ("Direct rotary handle NSX100-250 black",
     "Poign\u00e9e rotative directe NSX100-250 noire"), 0, [],
    "handle", body=ACC_BODY)
add("accessories", "terminal_shield_lv429517", "LV429517",
    "Long terminal shield, ComPacT NSX100/160/250, 3P",
    "Long terminal shield for ComPacT NSX100/160/250, 3P, pole pitch "
    "35 mm, degree of protection IP40. Mechanical accessory, no terminal.",
    ("Long terminal shield NSX100-250 3P",
     "Cache-bornes long NSX100-250 3P"), 0, [], "shield", body=ACC_BODY)

# TeSys D contactors, 3 poles, 1 NO + 1 NC built in (verified for D09 and
# D38).  The width of 45 mm is verified for D09 and D38; the height of 77 mm
# and the depth of 95 mm are verified for LC1D09BD only and ASSUMED for the
# other ratings.  Scale 3.33 px/mm: 45 mm = 150 px.
CONTACTOR_POWER = [("1/L1", "2/T1"), ("3/L2", "4/T2"), ("5/L3", "6/T3")]
for amp, kw in ((9, "4"), (12, "5.5"), (18, "7.5"), (25, "11"),
                (32, "15"), (38, "18.5")):
    add("tesys_d", "lc1d%02dp7" % amp, "LC1D%02dP7" % amp,
        "TeSys D contactor, 3P, %d A AC-3, 230 V AC coil, 1 NO + 1 NC" % amp,
        "TeSys D contactor LC1D%02dP7, 3 poles, %d A AC-3 (%s kW at 400 V), "
        "coil 230 V AC 50/60 Hz, 1 NO + 1 NC auxiliary contact built in, "
        "screw clamp terminals, 45 mm wide. Terminals 1/L1-2/T1, 3/L2-4/T2, "
        "5/L3-6/T3, coil A1-A2, auxiliary 13-14 (NO) and 21-22 (NC)." % (
            amp, amp, kw),
        ("TeSys D contactor LC1D%02dP7 %d A 230 V AC" % (amp, amp),
         "Contacteur TeSys D LC1D%02dP7 %d A 230 V CA" % (amp, amp)),
        3, CONTACTOR_POWER, "contactor", xs=[0, 50, 100], body=(-20, 130),
        height=260, rating="%d A" % amp, coil="230 V AC", model="LC1D%02d" %
        amp, wmm=45, hmm=77, dmm=95)
add("tesys_d", "lc1d09bd", "LC1D09BD",
    "TeSys D contactor, 3P, 9 A AC-3, 24 V DC coil, 1 NO + 1 NC",
    "TeSys D contactor LC1D09BD, 3 poles, 9 A AC-3 (4 kW at 400 V), coil "
    "24 V DC, 1 NO + 1 NC auxiliary contact built in, screw clamp terminals, "
    "45 x 77 x 95 mm. Terminals 1/L1-2/T1, 3/L2-4/T2, 5/L3-6/T3, coil "
    "A1-A2, auxiliary 13-14 (NO) and 21-22 (NC).",
    ("TeSys D contactor LC1D09BD 9 A 24 V DC",
     "Contacteur TeSys D LC1D09BD 9 A 24 V CC"),
    3, CONTACTOR_POWER, "contactor", xs=[0, 50, 100], body=(-20, 130),
    height=260, rating="9 A", coil="24 V DC", model="LC1D09",
    wmm=45, hmm=77, dmm=95)

# Front-mounting auxiliary contact blocks LADN (verified: screw clamp
# terminals, for LC1D and LC1F, mirror contact).  Marking of the LADN22
# VERIFIED (public manufacturer FAQ): 53-54 NO, 61-62 NC, 71-72 NC, 83-84 NO
# from left to right.  The marking of the other variants is ASSUMED: the
# contacts of the LADN22 that they keep.
LADN = (("ladn22", "LADN22", "2 NO + 2 NC", (("53", "54", "no"),
                                              ("61", "62", "nc"),
                                              ("71", "72", "nc"),
                                              ("83", "84", "no"))),
        ("ladn11", "LADN11", "1 NO + 1 NC", (("53", "54", "no"),
                                              ("61", "62", "nc"))),
        ("ladn20", "LADN20", "2 NO", (("53", "54", "no"),
                                      ("83", "84", "no"))),
        ("ladn02", "LADN02", "2 NC", (("61", "62", "nc"),
                                      ("71", "72", "nc"))))
LADN_COLUMNS = {"53": 0, "61": 1, "71": 2, "83": 3}
for stem, ref, contacts, blocks in LADN:
    add("tesys_d_accessories", stem, ref,
        "TeSys D front auxiliary contact block %s, %s, screw clamp" % (
            ref, contacts),
        "Front-mounting instantaneous auxiliary contact block %s for TeSys "
        "D (LC1D) and TeSys F contactors, %s, screw clamp terminals. "
        "Terminal marking %s.%s" % (
            ref, contacts, ", ".join("%s-%s" % (a, b)
                                     for a, b, _ in blocks),
            "" if ref == "LADN22" else " ASSUMED."),
        ("TeSys D auxiliary contact block %s %s" % (ref, contacts),
         "Bloc de contacts auxiliaires TeSys D %s %s" % (ref, contacts)),
        0, [(a, b) for a, b, _ in blocks], "ladn", body=(-15, 135),
        xs=[40 * LADN_COLUMNS[a] for a, _, _ in blocks], contacts=blocks,
        wmm=45, hmm=40, dmm=None)


# ------------------------------------------------ compatibility lists
# Accessories that the public product pages of the manufacturer give as
# compatible, by parent stem, in order of priority.  The element carries
# the first MAX_BLOCKS groups as auxiliary article blocks with quantity 0
# (the Articles tab has four blocks); the project tool
# qet_accessory_articles.py uses the whole list.  A device absent from this
# table has NO verified list: nothing may be linked to it.
MAX_BLOCKS = 4
IOF = "A9A26924"
NSX_OFF, NSX_MN, NSX_HANDLE, NSX_SHIELD = ("29450", "LV429410", "LV429337",
                                           "LV429517")
MT_AC, MT_DC = "LV431541", "LV431543"
COMPAT = {
    "ic60n_1p": [IOF], "ic60n_2p": [IOF], "ic60n_4p": [IOF],
    "ic60n_3p": [IOF, "A9V11325"],
    "idpna_vigi_1pn": [IOF], "iid_2p": [IOF], "iid_4p": [IOF],
    "isw_3p_40a": [IOF],
    "nsx100b_3p_tmd100": [NSX_OFF, NSX_MN, NSX_HANDLE, NSX_SHIELD],
    "nsx250n_3p_frame": [MT_AC, NSX_OFF, NSX_MN, NSX_SHIELD, MT_DC,
                         NSX_HANDLE],
    "nsx250n_3p_tm250": [MT_AC, NSX_OFF, NSX_MN, NSX_SHIELD, MT_DC,
                         NSX_HANDLE],
}
# TeSys D contactors: the front auxiliary blocks LADN (public product pages:
# for LC1D and LC1F contactors)
for _amp in (9, 12, 18, 25, 32, 38):
    COMPAT["lc1d%02dp7" % _amp] = ["LADN22", "LADN11", "LADN20", "LADN02"]
COMPAT["lc1d09bd"] = ["LADN22", "LADN11", "LADN20", "LADN02"]
# Alternatives that exclude each other share a group (one motor mechanism)
GROUPS = {MT_DC: MT_AC}


def group(ref):
    return GROUPS.get(ref, ref)


def seeded(refs):
    """The references that are written as quantity 0 blocks: one per group,
    the first MAX_BLOCKS."""
    out = []
    for ref in refs:
        if group(ref) == ref and len(out) < MAX_BLOCKS:
            out.append(ref)
    return out


def product_by_reference():
    return {p.reference: p for p in PRODUCTS}


# -------------------------------------------------------- IEC symbols

SYM_PITCH = 30


def symbol_pole(sh, x, kind, top, bottom):
    """One pole, top terminal (x, 0), bottom terminal (x, 60)."""
    sh.line(x, 0, x, 14, "normal")
    sh.line(x, 40, x, 60, "normal")
    sh.line(x, 40, x - 8, 14, "normal")
    if kind == "breaker":
        sh.line(x - 12, 10, x - 4, 18, "normal")      # contact cross
        sh.line(x - 12, 18, x - 4, 10, "normal")
        sh.rect(x - 4, 0, 8, 14, "none")             # overload release
    elif kind in ("switch", "ins"):
        sh.line(x - 13, 11, x - 3, 17, "normal")     # isolator bar
    elif kind == "fuse":
        sh.rect(x - 5, 0, 10, 14, "white")
        sh.line(x, 0, x, 14, "thin")
        sh.line(x - 13, 11, x - 3, 17, "normal")
    elif kind == "nc":
        sh.line(x - 8, 14, x, 14, "normal")          # closed contact bar
    sh.terminal(top, x, 0, "n")
    sh.terminal(bottom, x, 60, "s")


def number_pair(sh, x, top, bottom):
    """The terminal numbers written beside a pole."""
    mark(sh, top, x, 8)
    mark(sh, bottom, x, 52)


def draw_symbol(p):
    sh = Sheet()
    xs = [SYM_PITCH * k for k in range(len(p.labels))]
    kind = {"rcbo": "breaker", "rcd": "switch", "mccb": "breaker"}.get(
        p.kind, p.kind)
    for x, (top, bottom) in zip(xs, p.labels):
        symbol_pole(sh, x, kind, top, bottom)
    if len(xs) > 1:
        sh.line(xs[0] - 8, 27, xs[-1] - 8, 27, dashed=True)
    if p.kind in ("rcd", "rcbo"):
        # core of the differential block around the poles, test line
        sh.circle((xs[0] + xs[-1]) / 2.0, 50, 12, "none")
        sh.line(xs[0], 50, xs[-1] + 14, 50, dashed=True)
    sh.grow(xs[0] - 20, -10, xs[-1] + 20, 70)
    return sh


def aux_symbol():
    """One changeover contact: common 11, NC 12, NO 14 (ASSUMED marking of
    the first slot, see docs/schneider.md)."""
    sh = Sheet()
    sh.line(0, 0, 0, 12, "normal")
    sh.line(0, 12, -8, 36, "normal")
    sh.line(0, 60, 0, 48, "normal")
    sh.line(30, 60, 30, 48, "normal")
    sh.line(30, 48, 22, 24, "normal")
    sh.terminal("11", 0, 0, "n")
    sh.terminal("12", 0, 60, "s")
    sh.terminal("14", 30, 60, "s")
    mark(sh, "11", 0, 8)
    mark(sh, "12", 0, 52)
    mark(sh, "14", 30, 52)
    sh.grow(-20, -10, 50, 70)
    return sh


def contactor_symbol(p):
    """Three NO power poles, the coil A1-A2 and the built-in 13-14 (NO) and
    21-22 (NC) auxiliary contacts, joined by a dashed mechanical link."""
    sh = Sheet()
    xs = [SYM_PITCH * k for k in range(3)]
    for x, (top, bottom) in zip(xs, p.labels):
        symbol_pole(sh, x, "no", top, bottom)
        number_pair(sh, x, top, bottom)
    sh.line(-8, 27, 172, 27, dashed=True)
    sh.line(110, 0, 110, 20, "normal")
    sh.line(110, 40, 110, 60, "normal")
    sh.rect(100, 20, 20, 20, "white", "normal")
    sh.terminal("A1", 110, 0, "n")
    sh.terminal("A2", 110, 60, "s")
    mark(sh, "A1", 110, 8)
    mark(sh, "A2", 110, 52)
    for x, kind, (top, bottom) in ((150, "no", ("13", "14")),
                                   (180, "nc", ("21", "22"))):
        symbol_pole(sh, x, kind, top, bottom)
        number_pair(sh, x, top, bottom)
    sh.grow(-20, -10, 210, 70)
    return sh


def ladn_symbol(p):
    """The contacts of a front auxiliary block, in the order of the block."""
    sh = Sheet()
    for k, (top, bottom, kind) in enumerate(p.contacts):
        x = SYM_PITCH * k
        symbol_pole(sh, x, kind, top, bottom)
        number_pair(sh, x, top, bottom)
    n = len(p.contacts)
    if n > 1:
        sh.line(-8, 27, SYM_PITCH * (n - 1) - 8, 27, dashed=True)
    sh.grow(-20, -10, SYM_PITCH * (n - 1) + 40, 70)
    return sh


def coil_symbol(kind):
    sh = Sheet()
    sh.line(0, 0, 0, 20, "normal")
    sh.line(0, 40, 0, 60, "normal")
    if kind == "motor":
        sh.circle(0, 30, 20, "white", "normal")
        sh.text(0, 30, "M", 8, True)
    else:
        sh.rect(-10, 20, 20, 20, "white", "normal")
        sh.text(0, 30, "U<", 6, True)
    sh.terminal("M1" if kind == "motor" else "D1", 0, 0, "n")
    sh.terminal("M2" if kind == "motor" else "D2", 0, 60, "s")
    sh.grow(-20, -10, 20, 70)
    return sh


# ------------------------------------------------------ realistic views

def modular_front(p):
    """Acti9 front: light grey body, grey terminal zones with silver screws,
    black toggle on a grey base, red position window, white label, green
    accent band, DIN rail clip.  Colours ASSUMED, see docs/schneider.md."""
    sh = Sheet()
    bx0, bx1 = p.body
    top, bot = -20, -20 + p.height
    w = bx1 - bx0
    cx = (p.xs[0] + p.xs[-1]) / 2.0
    sh.rect(bx0, top, w, p.height, LIGHT, "normal", frame=True)
    sh.rect(bx0, top, w, 30, MID)
    sh.rect(bx0, bot - 30, w, 30, MID)
    sh.rect(bx0, 30, w, 6, GREEN)                # colour accent band
    sh.rect(cx - 25, bot - 8, 50, 8, DARK)       # DIN rail clip
    for k in range(1, len(p.xs)):                # joint between two poles
        sh.line((p.xs[k - 1] + p.xs[k]) / 2.0, top + 30,
                (p.xs[k - 1] + p.xs[k]) / 2.0, bot - 30)
    for x, (t, b) in zip(p.xs, p.labels):
        screw(sh, x, 0)
        screw(sh, x, bot - 20)
        sh.terminal(t, x, 0, "n")
        sh.terminal(b, x, bot - 20, "s")
    if p.kind == "fuse":
        for x in p.xs:                      # fuse carrier of each pole
            sh.rect(x - 22, 50, 44, 140, MID, "normal")
            sh.line(x - 22, 90, x + 22, 90)
            sh.rect(x - 8, 50, 16, 20, BLACK)
            sh.text(x, 140, p.rating, 7, True)
        sh.text(cx, 215, "10.3x38", 5)
    else:
        if p.kind in ("switch", "rcd"):     # one common handle
            handles = [(bx0 + 10, w - 20)]
        elif p.kind == "rcbo":
            handles = [(cx - 22, 44)]
        else:                               # one handle per pole
            handles = [(x - 22, 44) for x in p.xs]
        for x, width in handles:
            sh.rect(x - 4, 50, width + 8, 100, MID, "normal")   # base
            sh.rect(x, 56, width, 52, BLACK)                    # toggle
            for y in (64, 74, 84, 94):                          # grip
                sh.rect(x + 4, y, width - 8, 3, DARK)
            sh.rect(x, 38, width, 10, ON_RED)    # contact position: ON
        if p.kind in ("rcd", "rcbo"):
            sh.rect(bx0 + 4, 150, w - 8, 70, WHITE, "normal")
            sh.circle(p.xs[0], 172, 18, WARN, "normal")
            sh.text(p.xs[0], 172, "T", 7, True)
            sh.rect(p.xs[-1] - 8, 160, 16, 8, ON_RED)  # fault indicator
            sh.text(cx, 197, "%s %s" % (p.sens, p.rtype), 5, True)
            sh.text(cx, 209, p.rating, 5)
            sh.text(cx, 238, p.model, 6, True)
        else:
            sh.rect(p.xs[0] - 22, 175, 44, 28, WHITE, "normal")
            sh.text(p.xs[0], 189, p.rating, 8, True)
            sh.text(cx, 215, p.model, 6, True)
    return sh


def toggle_and_trip(sh, p, cx, bx0, bx1):
    sh.rect(cx - 45, 90, 90, 150, BLACK, "normal")
    sh.rect(cx - 22, 70, 44, 120, BLACK, "normal")
    for y in (95, 110, 125, 140):                     # grip
        sh.rect(cx - 16, y, 32, 4, DARK)
    sh.text(cx - 70, 100, "I", 9, True)
    sh.text(cx - 70, 215, "O", 9, True)
    sh.rect(bx0 + 35, 265, bx1 - bx0 - 70, 100, WHITE, "normal")
    if p.trip:
        sh.circle(cx - 60, 315, 56, LIGHT, "normal")
        sh.line(cx - 60, 315, cx - 40, 295, "normal")
        sh.text(cx + 40, 300, p.trip, 8, True)
        sh.text(cx + 40, 330, p.rating, 8)
    else:
        sh.text(cx, 300, "no trip unit", 7)
    sh.circle(bx1 - 55, 285, 16, ON_RED, "normal")          # push to trip
    sh.text(cx, 385, "%s 3P" % p.model, 9, True)


def rotary_handle(sh, cx):
    sh.circle(cx, 215, 220, LIGHT, "normal")
    sh.circle(cx, 215, 190, WHITE)
    sh.rect(cx - 95, 180, 190, 70, BLACK, "normal")
    sh.rect(cx + 55, 180, 40, 70, GREEN)
    sh.text(cx - 80, 130, "O", 10, True)
    sh.text(cx + 80, 130, "I", 10, True)


def shields(sh, bx0, bx1, xs, frame):
    for y0 in (-100, 425):
        sh.rect(bx0 + 10, y0, bx1 - bx0 - 20, 75, "none", "normal",
                frame=frame)
        for x in xs:
            sh.rect(x - 24, y0 + 20, 48, 36, "none")
        for x in (xs[0] + 60, xs[-1] - 60):
            sh.line(x, y0, x, y0 + 75)


def motor_module(sh, bx0, bx1, cx, frame):
    sh.rect(bx0 + 10, 40, bx1 - bx0 - 20, 215, LIGHT, "normal",
            frame=frame)
    sh.rect(cx - 55, 60, 110, 170, MID, "normal")
    sh.rect(cx - 22, 75, 44, 110, BLACK)
    sh.rect(cx - 22, 75, 44, 14, GREEN)
    sh.circle(bx0 + 70, 215, 36, WHITE, "normal")           # auto/manual
    sh.line(bx0 + 70, 215, bx0 + 86, 199, "normal")
    sh.text(bx0 + 70, 245, "A/M", 5)
    sh.text(bx1 - 65, 215, "MT250", 6, True)


def of_flag(sh, bx0, frame):
    sh.rect(bx0 + 10, 58, 60, 40, GREEN, "normal", frame=frame)
    sh.text(bx0 + 40, 66, "OF/SD", 5, True)


def mn_flag(sh, bx1, frame):
    sh.rect(bx1 - 70, 58, 60, 26, WARN, "normal", frame=frame)
    sh.text(bx1 - 40, 71, "MN", 5, True)


def nsx_front(p, with_accessories=False):
    """ComPacT NSX / INS front: dark grey case, grey terminal blocks, black
    toggle or rotary handle, white trip unit window, red push-to-trip,
    green accent band.  Colours ASSUMED, see docs/schneider.md."""
    sh = Sheet()
    bx0, bx1 = p.body
    w = bx1 - bx0
    cx = (p.xs[0] + p.xs[-1]) / 2.0
    sh.rect(bx0, NSX_TOP, w, NSX_BOTTOM - NSX_TOP, DARK, "normal",
            frame=True)
    for x, (t, b) in zip(p.xs, p.labels):
        sh.rect(x - 40, -35, 80, 70, MID)
        sh.rect(x - 40, 395, 80, 70, MID)
        screw(sh, x, 0, 30)
        screw(sh, x, 430, 30)
        sh.terminal(t, x, 0, "n")
        sh.terminal(b, x, 430, "s")
    sh.rect(bx0 + 20, 60, w - 40, 310, MID, "normal")
    sh.rect(bx0 + 20, 60, w - 40, 10, GREEN)             # accent band
    if p.kind == "ins":
        rotary_handle(sh, cx)
        sh.text(cx, 345, "%s %s" % (p.model, p.rating), 9, True)
    else:
        toggle_and_trip(sh, p, cx, bx0, bx1)
    if with_accessories:
        shields(sh, bx0, bx1, p.xs, True)
        motor_module(sh, bx0, bx1, cx, False)
        of_flag(sh, bx0, False)
        mn_flag(sh, bx1, False)
    return sh


def iof_front(p):
    """Acti9 auxiliary contact: one module wide, terminals 11, 12, 14 on its
    top."""
    sh = Sheet()
    sh.rect(-15, -10, 30, 140, LIGHT, "normal", frame=True)
    sh.rect(-15, 100, 30, 30, MID)
    sh.rect(-15, 20, 30, 5, GREEN)
    sh.rect(-10, 40, 20, 40, WHITE, "normal")
    sh.rect(-6, 46, 12, 10, GREEN)
    for name, x in (("12", -10), ("11", 0), ("14", 10)):
        screw(sh, x, 0, 8)
        sh.terminal(name, x, 0, "n")
        sh.text(x, 12, name, 4)
    sh.grow(0, 0, 0, 0)
    return sh


def accessory_front(p):
    """An accessory at its place on the front of an NSX250 3P."""
    sh = Sheet()
    bx0, bx1 = p.body
    cx = 120
    if p.kind == "motor":
        motor_module(sh, bx0, bx1, cx, True)
        sh.text(cx, 150, p.supply, 6)
        sh.terminal("M1", 100, 250, "s")
        sh.terminal("M2", 140, 250, "s")
    elif p.kind == "mn":
        mn_flag(sh, bx1, True)
        sh.terminal("D1", 250, 80, "s")
        sh.terminal("D2", 270, 80, "s")
    elif p.kind == "aux":
        of_flag(sh, bx0, True)
        for name, x in (("11", -40), ("12", -20), ("14", 0)):
            sh.terminal(name, x, 90, "s")
            sh.circle(x, 90, 6, SILVER, "normal")
            sh.text(x, 80, name, 4)
    elif p.kind == "handle":
        rotary_handle(sh, cx)
        sh.grow(cx - 110, 105, cx + 110, 325)
    elif p.kind == "shield":
        shields(sh, bx0, bx1, (0, 120, 240), True)
    if sh.terminals:
        sh.grow(0, 0, 0, 0)     # the origin on the frame, see the grid check
    return sh


def contactor_front(p):
    """TeSys D front: dark grey case, black terminal zones with silver
    screws and white numbers, light front plate with a white coil window,
    a green accent band and a black test button.  The side terminals
    (A1, A2, 13, 14, 21, 22) are a generic ASSUMED layout."""
    sh = Sheet()
    sh.rect(-20, -30, 150, 260, DARK, "normal", frame=True)
    sh.rect(-20, -30, 150, 60, BLACK)
    sh.rect(-20, 170, 150, 60, BLACK)
    for x, (t, b) in zip(p.xs, p.labels):
        screw(sh, x, 0, 16)
        screw(sh, x, 200, 16)
        sh.terminal(t, x, 0, "n")
        sh.terminal(b, x, 200, "s")
        sh.text(x, 20, t, 5, False, "#ffffff")
        sh.text(x, 182, b, 5, False, "#ffffff")
    sh.rect(0, 40, 110, 120, LIGHT, "normal")
    sh.rect(0, 40, 110, 8, GREEN)                       # accent band
    sh.rect(10, 58, 90, 36, WHITE, "normal")            # coil window
    sh.text(55, 70, p.coil, 7, True)
    sh.text(55, 85, p.model, 6)
    sh.rect(30, 108, 50, 40, MID, "normal")             # test button
    sh.rect(40, 116, 30, 24, BLACK)
    for name, x, y, ori, side in (("A1", -20, 50, "w", "r"),
                                  ("A2", -20, 180, "w", "r"),
                                  ("13", 130, 50, "e", "l"),
                                  ("14", 130, 180, "e", "l"),
                                  ("21", 130, 90, "e", "l"),
                                  ("22", 130, 140, "e", "l")):
        screw(sh, x, y, 10)
        sh.terminal(name, x, y, ori)
        mark(sh, name, x, y, 5, side, "#ffffff")
    return sh


def ladn_front(p):
    """LADN block: a dark grey housing with one screw pair per contact and
    a light label plate; an NO contact has a green tag, an NC contact a red
    one.  Dimensions ASSUMED."""
    sh = Sheet()
    dx = -min(p.xs)         # the origin is on the first terminal
    sh.rect(-15 + dx, -30, 150, 130, DARK, "normal", frame=True)
    sh.rect(-15 + dx, -30, 150, 40, BLACK)
    sh.rect(-15 + dx, 60, 150, 40, BLACK)
    sh.rect(-5 + dx, 20, 130, 30, LIGHT, "normal")
    sh.rect(-5 + dx, 20, 130, 5, GREEN)
    for x, (top, bottom, kind) in zip(p.xs, p.contacts):
        x += dx
        screw(sh, x, 0, 16)
        screw(sh, x, 70, 16)
        sh.terminal(top, x, 0, "n")
        sh.terminal(bottom, x, 70, "s")
        sh.rect(x - 10, 34, 20, 8, GREEN if kind == "no" else ON_RED)
        sh.text(x, -18, top, 5, False, "#ffffff")
        sh.text(x, 88, bottom, 5, False, "#ffffff")
    return sh


# ---------------------------------------------------------------- output

def names_xml(p, suffix):
    en, fr = p.names
    extra = {"": ("", ""),
             "_realistic": (", front view", ", vue de face"),
             "_accessories": (", front view with accessories",
                              ", vue de face avec accessoires")}[suffix]
    return ('\t<names>\n\t\t<name lang="en">%s</name>\n'
            '\t\t<name lang="fr">%s</name>\n\t</names>' % (
                esc(en + extra[0]), esc(fr + extra[1])))


def element_xml(p, sh, suffix, note):
    frame = sh.bbox
    x0, y0 = down10(frame[0]), down10(frame[1])
    x1, y1 = up10(frame[2]), up10(frame[3])
    out = ['<definition version="0.100.0" type="element" link_type="simple" '
           'width="%d" height="%d" hotspot_x="%d" hotspot_y="%d">' % (
               x1 - x0, y1 - y0, -x0, -y0),
           '\t<uuid uuid="%s" />' % uid(p.stem, "view", suffix),
           names_xml(p, suffix), "\t<elementInformations>"]
    for name, value in (("manufacturer", MANUFACTURER),
                        ("manufacturer_reference", p.reference),
                        ("designation", p.designation),
                        ("description", p.description)):
        out.append('\t\t<elementInformation name="%s" show="1">%s'
                   "</elementInformation>" % (name, esc(value)))
    refs = product_by_reference()
    for n, ref in enumerate(seeded(COMPAT.get(p.stem, [])), 1):
        acc = refs[ref]
        for name, value in (("auxiliary%d" % n, "Compatible accessory"),
                            ("description_auxiliary%d" % n,
                             acc.description),
                            ("designation_auxiliary%d" % n,
                             acc.designation),
                            ("manufacturer_auxiliary%d" % n, MANUFACTURER),
                            ("manufacturer_reference_auxiliary%d" % n,
                             acc.reference),
                            ("quantity_auxiliary%d" % n, "0")):
            out.append('\t\t<elementInformation name="%s" show="0">%s'
                       "</elementInformation>" % (name, esc(value)))
    out.append("\t</elementInformations>")
    out.append("\t<informations>%s\n%s</informations>" % (esc(INFO),
                                                         esc(note)))
    out.append("\t<description>")
    for part in sh.parts:
        out.append("\t\t" + part)
    for name, x, y, ori in sh.terminals:
        out.append('\t\t<terminal x="%s" y="%s" name="%s" orientation="%s" '
                   'type="Generic" uuid="%s" />' % (
                       num(x), num(y), esc(name), ori,
                       uid(p.stem, suffix, "t", name)))
    out.append(
        '\t\t<dynamic_text x="%s" y="%s" z="25" text_width="-1" '
        'Halignment="AlignLeft" Valignment="AlignTop" frame="false" '
        'rotation="0" keep_visual_rotation="true" text_from="ElementInfo" '
        'uuid="%s" font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular">\n'
        '\t\t\t<text />\n\t\t\t<info_name>label</info_name>\n'
        '\t\t</dynamic_text>' % (num(x0 + 5), num(y1 + 3),
                                 uid(p.stem, suffix, "label")))
    out.append("\t</description>")
    out.append("</definition>")
    return "\n".join(out) + "\n"


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def directory_xml(names):
    en, fr, de = names
    return ('<qet-directory>\n\t<names>\n\t\t<name lang="en">%s</name>\n'
            '\t\t<name lang="fr">%s</name>\n\t\t<name lang="de">%s</name>\n'
            "\t</names>\n</qet-directory>\n" % (esc(en), esc(fr), esc(de)))


def build(p):
    """The views of a product: suffix -> Sheet."""
    files = {}
    if p.kind == "contactor":
        files[""] = contactor_symbol(p)
        files["_realistic"] = contactor_front(p)
    elif p.kind in ("breaker", "rcbo", "rcd", "switch", "fuse", "mccb",
                    "ins"):
        files[""] = draw_symbol(p)
        files["_realistic"] = (nsx_front(p) if p.kind in ("mccb", "ins")
                               else modular_front(p))
        if p.accessories:
            files["_accessories"] = nsx_front(p, True)
    else:
        if p.kind in ("motor", "mn"):
            files[""] = coil_symbol(p.kind)
        elif p.kind in ("aux", "iof"):
            files[""] = aux_symbol()
        elif p.kind == "ladn":
            files[""] = ladn_symbol(p)
        files["_realistic"] = (iof_front(p) if p.kind == "iof" else
                               ladn_front(p) if p.kind == "ladn" else
                               accessory_front(p))
    return files


def main():
    out_dir = os.path.normpath(sys.argv[1])
    if os.path.basename(out_dir) != "schneider":
        sys.exit("the output folder must be named schneider")
    if os.path.isdir(out_dir):
        shutil.rmtree(out_dir)
    for folder, names in DIRECTORIES.items():
        write(os.path.join(out_dir, folder, "qet_directory"),
              directory_xml(names))
    for p in PRODUCTS:
        scale = ("scale 3.33 px/mm (1 module of 9 mm = 30 px)"
                 if p.folder.startswith(("acti9", "tesys")) else
                 "scale 3.43 px/mm (pole pitch 35 mm = 120 px)")
        note = ("Made by misc/schneider/gen_schneider.py: %s, grid 10 px. %s"
                % (scale, LICENSE))
        for suffix, sh in build(p).items():
            write(os.path.join(out_dir, p.folder, p.stem,
                               p.stem + suffix + ".elmt"),
                  element_xml(p, sh, suffix, note))
        write(os.path.join(out_dir, p.folder, p.stem, "qet_directory"),
              directory_xml((p.names[0], p.names[1], p.names[0])))
    print("wrote %d products" % len(PRODUCTS))


if __name__ == "__main__":
    main()
