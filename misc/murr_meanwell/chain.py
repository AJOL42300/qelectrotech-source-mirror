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
"""Follow a function (signal) through the passive M12 elements.

The moulded cables, T couplers and Y distributors only pass the channels
of an M12 socket on, following the links of gen_m12.ROUTES. A walk starts
on a terminal and a channel, crosses the conductors and the passive
elements, and returns the (label, terminal, channel) where it ends: an
element that is not passive. A channel without link in a passive element
breaks the chain and the walk returns nothing for it. The channels never
get mixed.

The same rules are written in C++ in tst_murrmeanwellelements.cpp.
"""


def walk(start, conductors, family_of, routes):
    """start: (label, terminal, channel). conductors: iterable of
    ((label, terminal), (label, terminal)). family_of: label -> family of
    a passive element (labels not in it are not passive). routes: family
    -> list of (terminal, channel, terminal, channel)."""
    nxt = {}
    for a, b in conductors:
        nxt.setdefault(a, []).append(b)
        nxt.setdefault(b, []).append(a)
    ends, seen = [], set()
    todo = [start]
    while todo:
        label, term, ch = todo.pop()
        if (label, term, ch) in seen:
            continue
        seen.add((label, term, ch))
        for l2, t2 in nxt.get((label, term), []):
            family = family_of.get(l2)
            if family is None:
                ends.append((l2, t2, ch))
                continue
            for ta, ca, tb, cb in routes.get(family, []):
                if (t2, ch) == (ta, ca):
                    todo.append((l2, tb, cb))
                elif (t2, ch) == (tb, cb):
                    todo.append((l2, ta, ca))
    return sorted(ends)
