/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "pinusage.h"

#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QCoreApplication>
#include <QHash>
#include <QSettings>
#include <QVector>
#include <algorithm>

namespace {

	struct Placement {
		const Element *element;
		int folio;
	};

	/// label of the module -> pin name -> where it is wired
	using Usage = QHash<QString, QHash<QString, QVector<Placement>>>;

	struct Cache {
		quint64 revision = 0;
		Usage usage;
	};

	quint64 g_revision = 1;
	int g_grey = -1;
	QHash<const QETProject *, Cache> g_caches;

	QString labelOf(const Element *element)
	{
		return element->ownInformations()
			.value(QStringLiteral("label")).toString();
	}

	const Usage &usageOf(const QETProject *project)
	{
		Cache &cache = g_caches[project];
		if (cache.revision == g_revision) return cache.usage;
		cache.usage.clear();
		cache.revision = g_revision;
		for (const Diagram *diagram : project->diagrams()) {
			const int folio = project->folioIndex(diagram) + 1;
			for (const Element *element : diagram->elements()) {
				const QString label = labelOf(element);
				if (label.isEmpty()) continue;
				for (const Terminal *terminal : element->terminals()) {
					const QString name = terminal->baseName();
					if (name.isEmpty() || terminal->conductors().isEmpty())
						continue;
					cache.usage[label][name].append({element, folio});
				}
			}
		}
		return cache.usage;
	}

	const QVector<Placement> *placementsOf(const Element *element,
					       const QString &pin_name)
	{
		if (!element || !element->diagram()) return nullptr;
		const QETProject *project = element->diagram()->project();
		const QString label = labelOf(element);
		if (!project || label.isEmpty()) return nullptr;
		const Usage &usage = usageOf(project);
		const auto module = usage.constFind(label);
		if (module == usage.constEnd()) return nullptr;
		const auto pin = module->constFind(pin_name);
		return pin == module->constEnd() ? nullptr : &pin.value();
	}
}

bool PinUsage::greyUsedPinsIsOn()
{
	if (g_grey < 0) {
		g_grey = QSettings().value(
			QStringLiteral("diagrameditor/grey-used-pins"),
			false).toBool() ? 1 : 0;
	}
	return g_grey == 1;
}

void PinUsage::setGreyUsedPins(bool on)
{
	g_grey = on ? 1 : 0;
	invalidate();
}

void PinUsage::invalidate()
{
	++g_revision;
}

bool PinUsage::usedElsewhere(const Terminal *terminal)
{
	if (!terminal || !terminal->conductors().isEmpty()) return false;
	const Element *own = terminal->parentElement();
	const QVector<Placement> *placements =
		placementsOf(own, terminal->baseName());
	if (!placements) return false;
	for (const Placement &placement : *placements) {
		if (placement.element != own) return true;
	}
	return false;
}

QStringList PinUsage::foliosOf(const Element *element, const QString &pin_name)
{
	QList<int> folios;
	if (const QVector<Placement> *placements =
			placementsOf(element, pin_name)) {
		for (const Placement &placement : *placements) {
			if (!folios.contains(placement.folio))
				folios << placement.folio;
		}
	}
	std::sort(folios.begin(), folios.end());
	QStringList texts;
	for (int folio : folios)
		texts << QCoreApplication::translate("PinUsage", "folio %1")
				.arg(folio);
	return texts;
}
