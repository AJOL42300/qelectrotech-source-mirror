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
"""Product data of the Kinco KS105 range and of the Kinco M2 HMI models.

Everything here comes from public documents of the manufacturer (product
pages and datasheets of the KS series and of the M2 HMI series, the
installation manuals of the MT070 and GL070E, the wiring pages of the KS105
datasheet published by a distributor).
Nothing is copied from a drawing: the drawings are made by gen_kinco.py.

What could not be checked is described factually and listed as uncertain in
docs/kinco.md. The marking of a terminal is its name in the schematic.
"""

PLC_DIMENSIONS = (25.4, 100.0, 84.5)  # W x H x D in mm, DIN 35 rail


class Term:
    """One connection point. role colours the drawing and tells the kind of
    channel: in, out, ai, ao, rtd, l+, l-, com, pe, 0v, 24v, gnd, data,
    bus, port."""

    def __init__(self, name, role, pin=None, signal=None, text=None):
        self.name = name
        self.role = role
        self.pin = pin
        self.signal = signal
        self.text = text or name


class Group:
    """A set of terminals drawn together. kind is block (screw terminals of
    the top or bottom row, zone says which), power, rs485, rj45, db9, rtd or
    single (a connector of which only the whole is wired)."""

    def __init__(self, title, kind, terms, side, zone="mid", title_fr=None):
        self.title = title
        self.title_fr = title_fr or title
        self.kind = kind
        self.terms = terms
        self.side = side
        self.zone = zone


class Product:
    def __init__(self, ref, family, kind, groups, designation, designation_fr,
                 dims, facts, aux, description):
        self.ref = ref
        self.id = ref.lower().replace("-", "_")
        self.family = family
        self.kind = kind  # plc, io or hmi
        self.groups = groups
        self.designation = designation
        self.designation_fr = designation_fr
        self.dims = dims
        self.facts = facts
        self.aux = aux  # (designation, description, quantity)
        self.description = description
        self.cutout = None
        self.screen = None

    def terms(self):
        return [t for g in self.groups for t in g.terms]

    def ios(self):
        """The channels of the I/O table: (type, address, [terminals])."""
        types = {"in": "entree_digitale", "out": "sortie_digitale",
                 "ai": "entree_analogique", "ao": "sortie_analogique",
                 "port": "entree_universelle"}
        out = []
        for g in self.groups:
            if g.kind == "rtd":
                chans = {}
                for t in g.terms:
                    chans.setdefault(t.signal, []).append(t.name)
                for addr, names in chans.items():
                    out.append(("entree_analogique", addr, names))
                continue
            for t in g.terms:
                if t.role in types:
                    out.append((types[t.role], t.signal or t.name,
                                [t.name]))
        return out


def power():
    return Group("Power", "power", [Term("PE", "pe"), Term("0V", "0v"),
                                    Term("24V", "24v")], "w",
                 title_fr="Alimentation")


def rs485():
    return Group("RS485", "rs485", [Term("RS485.B", "data"),
                                    Term("RS485.A", "data")], "w")


def rj45(title, terms, side="e"):
    return Group(title, "rj45", terms, side)


def rs232_port():
    return [Term("RS232.RXD", "data", 3), Term("RS232.TXD", "data", 4),
            Term("RS232.GND", "gnd", 6)]


def can_pins(prefix):
    return [Term(f"{prefix}.H", "data", 1), Term(f"{prefix}.L", "data", 2),
            Term(f"{prefix}.GND", "gnd", 3)]


def plc_inputs(title, common, group, n=8, side="w", zone="top"):
    terms = [Term(common, "l+")]
    terms += [Term(f"I{group}.{i}", "in") for i in range(n)]
    return Group(title, "block", terms, side, zone, "Entrées")


def plc_outputs(title, supply, group, n=8, side="e", zone="bottom"):
    terms = list(supply)
    terms += [Term(f"Q{group}.{i}", "out") for i in range(n)]
    return Group(title, "block", terms, side, zone, "Sorties")


CPU_FACTS = [
    "Compact PLC, KincoBuilder programming",
    "8 digital inputs 24 V DC (I0.0 to I0.7), sink type only",
    "8 transistor outputs 24 V DC, source type (Q0.0 to Q0.7)",
    "4 high speed counters up to 200 kHz",
    "4 pulse train / PWM outputs",
    "Interrupt inputs I0.0 to I0.3, real time clock",
    "Up to 14 expansion modules",
    "24 V DC (20.4 to 28.8 V), 5 W, IP20",
    "DIN 35 rail, 25.4 x 100 x 84.5 mm",
]


def cpu(ref, comm_groups, comm_facts, extra_desc):
    groups = [power(), plc_inputs("Inputs", "1L+", 0), rs485(),
              plc_outputs("Outputs", [Term("2L-", "l-"), Term("2L+", "l+")],
                          0)]
    groups += comm_groups
    ports = sum(1 for g in groups if g.kind == "rj45")
    aux = [
        ("RJ45 plug", "RJ45 plug for the cable of a PORT0, CAN or expansion "
         "socket", str(ports)),
        ("Screw terminal blocks", "Terminal blocks of the unit: inputs, "
         "outputs, power supply and RS485", "1"),
        ("DIN rail", "Symmetrical DIN rail 35 mm", "1"),
    ]
    desc = ("Kinco KS105 compact PLC, 8 digital inputs and 8 transistor "
            "outputs, RS232 and RS485 ports" + extra_desc + ". Terminal "
            "names are the markings of the unit; the I/O table lists the "
            "channels. Check the pinout against the datasheet of your unit.")
    return Product(ref, "ks105_cpu", "plc", groups,
                   "Compact PLC, 8 DI, 8 DO, RS232, RS485" + comm_facts[0],
                   "Automate compact, 8 E TOR, 8 S TOR, RS232, RS485"
                   + comm_facts[1], PLC_DIMENSIONS,
                   CPU_FACTS + comm_facts[2], aux, desc)


def build_cpus():
    return [
        cpu("KS105-16DT",
            [rj45("RS232 (PORT0)", rs232_port()),
             rj45("Expansion", [Term("EXP", "bus")])],
            ("", "", ["PORT0: RS232, Modbus RTU slave",
                      "PORT1: RS485, Modbus RTU master / slave",
                      "Expansion bus socket (RJ45)"]),
            ""),
        cpu("KS105C1-16DT",
            [rj45("RS232 (PORT0)", rs232_port()),
             rj45("CAN", can_pins("CAN"))],
            (", CAN", ", CAN", ["PORT0: RS232, Modbus RTU slave",
                                "PORT1: RS485, Modbus RTU master / slave",
                                "1 CAN port, CANopen master / slave"]),
            " and one CAN port"),
        cpu("KS105C2-16DT",
            [rj45("CAN1", can_pins("CAN1")),
             rj45("CAN2 / PORT0",
                  [Term("CAN2.H", "data", 1), Term("CAN2.L", "data", 2)]
                  + rs232_port())],
            (", 2 x CAN", ", 2 x CAN",
             ["PORT0: RS232, Modbus RTU slave",
              "PORT1: RS485, Modbus RTU master / slave",
              "CAN1 usable as expansion port, CAN2 Kinco motion / CANopen"]),
            " and two CAN ports"),
    ]


def module(ref, designation, designation_fr, groups, facts, desc):
    aux = [("Screw terminal blocks", "Terminal blocks of the module", "1"),
           ("DIN rail", "Symmetrical DIN rail 35 mm", "1")]
    common = ["RS485 port, Modbus RTU slave", "RUN and ERR indicators",
              "24 V DC (20.4 to 28.8 V), IP20",
              "DIN 35 rail, 25.4 x 100 x 84.5 mm"]
    return Product(ref, "ks_expansion", "io", [power(), rs485()] + groups,
                   designation, designation_fr, PLC_DIMENSIONS,
                   facts + common, aux,
                   f"Kinco {ref} expansion module for the KS series, "
                   f"{desc}. The terminal markings are logical names, check "
                   "them against the datasheet of your module.")


def build_modules():
    mods = []
    mods.append(module(
        "KS121-16DX", "Expansion module, 16 DI",
        "Module d'extension, 16 E TOR",
        [plc_inputs("Inputs 1", "1L+", 0),
         plc_inputs("Inputs 2", "2L+", 1, side="e", zone="bottom")],
        ["16 digital inputs, 8 per group"],
        "16 digital inputs in two groups of 8"))
    mods.append(module(
        "KS122-12XR", "Expansion module, 12 relay DO",
        "Module d'extension, 12 S relais",
        [Group("Relays 1", "block", [Term("1COM", "com")] +
               [Term(f"Q0.{i}", "out") for i in range(4)], "w", "top",
               "Relais 1"),
         Group("Relays 2", "block", [Term("2COM", "com")] +
               [Term(f"Q0.{i}", "out") for i in range(4, 8)], "e", "top",
               "Relais 2"),
         Group("Relays 3", "block", [Term("3COM", "com")] +
               [Term(f"Q1.{i}", "out") for i in range(4)], "e", "bottom",
               "Relais 3")],
        ["12 relay outputs, 4 per group", "2 A per output, 10 A per group",
         "DC 30 V / AC 250 V"],
        "12 relay outputs in three groups of 4"))
    mods.append(module(
        "KS122-14DT", "Expansion module, 14 transistor DO",
        "Module d'extension, 14 S transistor",
        [plc_outputs("Outputs 1", [Term("1L-", "l-"), Term("1L+", "l+")],
                     0, 8, "w", "top"),
         plc_outputs("Outputs 2", [Term("2L-", "l-"), Term("2L+", "l+")],
                     1, 6, "e", "bottom")],
        ["14 transistor outputs, 8 per group", "500 mA maximum per output"],
        "14 transistor outputs in two groups"))
    mods.append(module(
        "KS123-14DR", "Expansion module, 8 DI, 6 relay DO",
        "Module d'extension, 8 E TOR, 6 S relais",
        [plc_inputs("Inputs", "1L+", 0),
         Group("Relays", "block", [Term("COM", "com")] +
               [Term(f"Q0.{i}", "out") for i in range(6)], "e", "bottom",
               "Relais")],
        ["8 digital inputs, 6 relay outputs"],
        "8 digital inputs and 6 relay outputs"))
    mods.append(module(
        "KS133-06IV", "Expansion module, 4 AI, 2 AO",
        "Module d'extension, 4 E analogiques, 2 S analogiques",
        [Group("Analog inputs", "block",
               [Term(f"AI{i}", "ai") for i in range(4)]
               + [Term("AI.GND", "gnd")], "w", "top", "Entrées analogiques"),
         Group("Analog outputs", "block",
               [Term("AO0", "ao"), Term("AO1", "ao"), Term("AO.GND", "gnd")],
               "e", "bottom", "Sorties analogiques")],
        ["4 analog inputs: 4-20 mA, 1-5 V, 0-20 mA, 0-10 V, 12 bit",
         "2 analog outputs"],
        "4 analog inputs and 2 analog outputs"))
    rtd = []
    for c in range(4):
        rtd.append([Term(f"RTD{c}.{x}", "rtd", signal=f"RTD{c}")
                    for x in "ABC"])
    mods.append(module(
        "KS131-04RD", "Expansion module, 4 RTD inputs",
        "Module d'extension, 4 entrées sondes RTD",
        [Group("RTD 0-1", "rtd", rtd[0] + rtd[1], "w", "top", "RTD 0-1"),
         Group("RTD 2-3", "rtd", rtd[2] + rtd[3], "e", "bottom",
               "RTD 2-3")],
        ["4 RTD inputs: Pt100, Cu50, Pt1000, resistance",
         "2 and 3 wire, 24 bit"],
        "4 RTD temperature inputs, three terminals per channel"))
    return mods


M2_PAGE = "https://kincoautomation.ru/en/product/automation/m2/"

# The HMI of the library: the Kinco M2 series, models M2100E and M2070HE.
# ref, series, diag, resolution, ethernet, usb host, volts, watts, face,
# cut-out (width, height), source of the cut-out
HMI_ROWS = [
    ("M2070HE", "M2", 7.0, (1024, 600), 1, 1, "10.8-28", 4.2,
     (204, 150, 37.2), (192, 138),
     "https://dl.kinco-automation.com/HMI/manual/M2/Flyer_M2070HE.pdf and "
     + M2_PAGE + "mass-series-2-m2-m2070he-11860"),
    ("M2100E", "M2", 10.1, (1024, 600), 1, 1, "10.8-28", 6,
     (271, 191, 36), (260, 179),
     M2_PAGE + "mass-series-2-m2-m2100e-11863"),
]

# HMI Green GL and GH, taken out of the library on request (they were in
# the first version of this library, see the git history). Kept as data only.
# ref, series, diag, resolution, ethernet, usb host, volts, watts, face, cut
HMI_GREEN_ARCHIVED = [
    ("GH043", "GH", 4.3, (480, 272), 0, 1, "10.8-28", 3.5,
     (154, 89, 60), (146, 81)),
    ("GH043E", "GH", 4.3, (480, 272), 1, 1, "10.8-28", 2,
     (154, 89, 60), (146, 81)),
    ("GH070", "GH", 7.0, (800, 480), 0, 1, "12-28", 5,
     (207, 125, 41), (194, 113)),
    ("GH070E", "GH", 7.0, (800, 480), 1, 1, "12-28", 5,
     (207, 125, 41), (194, 113)),
    ("GH104E", "GH", 10.4, (800, 600), 1, 1, "10.8-28", 8,
     (310, 230, 56), (299, 219)),
    ("GH150E", "GH", 15.0, (1024, 768), 1, 1, "10.8-28", 16,
     (398, 298, 47), (383, 283)),
    ("GL043", "GL", 4.3, (480, 272), 0, 0, "10-28", 2,
     (132, 102, 33), (119, 93)),
    ("GL043E", "GL", 4.3, (480, 272), 1, 0, "10-28", 2,
     (132, 102, 33), (119, 93)),
    ("GL070", "GL", 7.0, (800, 480), 0, 1, "10-28", 3.6,
     (204, 150, 37.2), (192, 138)),
    ("GL070E", "GL", 7.0, (800, 480), 1, 1, "10-28", 3.6,
     (204, 150, 37.2), (192, 138)),
    ("GL100", "GL", 10.1, (1024, 600), 0, 1, "10-28", 6,
     (271, 191, 36), (260, 179)),
    ("GL100E", "GL", 10.1, (1024, 600), 1, 1, "10-28", 6,
     (271, 191, 36), (260, 179)),
    ("GL104E", "GL", 10.4, (800, 600), 1, 1, "10.8-28", 8,
     (310, 230, 56), (299, 219)),
    ("GL150E", "GL", 15.0, (1024, 768), 1, 1, "10.8-28", 16,
     (398, 298, 47), (383, 283)),
]

# DB9 pins of COM0 (GL070E installation manual): short signal names
DB9_SIGNALS = {1: "RX-/B", 2: "RXD", 3: "TXD", 4: "TX-", 5: "GND",
               6: "RX+/A", 7: "RXD-PC", 8: "TXD-PC", 9: "TX+"}


def com_group(n, pins, title, side):
    terms = [Term(f"COM{n}.{p}", "gnd" if DB9_SIGNALS[p] == "GND"
                  else "port", p, f"COM{n} {DB9_SIGNALS[p]}",
                  f"{p} {DB9_SIGNALS[p]}") for p in pins]
    return Group(title, "db9", terms, side)


def single(title, name, signal):
    return Group(title, "single",
                 [Term(name, "port", None, signal, signal)], "e")


def build_hmis():
    hmis = []
    for (ref, series, diag, res, eth, usbh, volts, watts, face, cut,
         source) in HMI_ROWS:
        groups = [Group("Power", "power", [Term("24V", "24v", 1),
                                           Term("0V", "0v", 2),
                                           Term("FG", "pe", 3)], "w",
                        title_fr="Alimentation"),
                  com_group(0, range(1, 10), "COM0 RS232/485/422", "w"),
                  Group("COM1 RS485", "pins",
                        [Term("COM1.1", "port", 1, "COM1 RX+/A",
                              "1 RX+/A"),
                         Term("COM1.2", "port", 2, "COM1 RX-/B",
                              "2 RX-/B")], "w"),
                  com_group(2, (2, 3, 5), "COM2 RS232", "e")]
        if eth:
            groups.append(single("Ethernet", "ETH", "Ethernet"))
        if usbh:
            groups.append(single("USB host", "USB.HOST", "USB host"))
        groups.append(single("USB slave", "USB.SLAVE", "USB slave"))
        aux = [("Power plug", "3-way screw terminal plug for 24 V, 0 V "
                "and FG", "1"),
               ("DB9 plug", "DB9 plug for a COM0 or COM2 port cable", "2"),
               ("COM1 plug", "2-pin plug for the RS485 cable of COM1", "1")]
        if eth:
            aux.append(("RJ45 plug", "RJ45 plug for the Ethernet cable",
                        "1"))
        aux.append(("USB cable", "USB cable for the host or slave port",
                    "1"))
        cut_text = f"{cut[0]} x {cut[1]} mm"
        facts = [f"Display {diag:g} in, {res[0]} x {res[1]}",
                 "COM0 RS232 / RS485 / RS422, COM1 RS485, COM2 RS232",
                 "Ethernet: " + ("1 x RJ45" if eth else "none"),
                 "USB host: " + ("1" if usbh else "none") + ", USB slave: 1",
                 f"Supply {volts} V DC, {watts:g} W",
                 f"Face {face[0]:g} x {face[1]:g} mm, depth {face[2]:g} mm",
                 f"Panel cut-out {cut_text} (W x H)",
                 "Front panel IP65"]
        desc = (f"Kinco HMI M2 series {ref}, {diag:g} inch {res[0]} x "
                f"{res[1]} operator panel. Panel cut-out (W x H): "
                f"{cut_text}, verified, source: {source}. The terminals "
                "are the power plug, the COM ports (pin number and signal), "
                "Ethernet and the USB ports; the pinout is the one of the "
                "MT070 / GL070E manuals and is assumed identical on this "
                "model, check it against the datasheet.")
        p = Product(ref, "hmi_m2", "hmi", groups,
                    f"HMI M2, {diag:g} in, {res[0]} x {res[1]}"
                    + (", Ethernet" if eth else ""),
                    f"IHM M2, {diag:g} pouces, {res[0]} x {res[1]}"
                    + (", Ethernet" if eth else ""),
                    face, facts, aux, desc)
        p.cutout = cut
        p.screen = (diag, res)
        p.volts = volts
        p.source = source
        hmis.append(p)
    return hmis


FAMILIES = {
    "ks105_cpu": ("KS105 CPU", "Automates KS105", "KS105 CPU"),
    "ks_expansion": ("KS expansion modules", "Modules d'extension KS",
                     "KS Erweiterungsmodule"),
    "hmi_m2": ("HMI M2 series (M2070HE, M2100E)",
               "IHM série M2 (M2070HE, M2100E)",
               "HMI M2-Serie (M2070HE, M2100E)"),
    "implantation": ("Layout (DIN rail, door cut-out)",
                     "Implantation (rail DIN, découpe de porte)",
                     "Layout (DIN-Schiene, Türausschnitt)"),
}


def all_products():
    return build_cpus() + build_modules() + build_hmis()
