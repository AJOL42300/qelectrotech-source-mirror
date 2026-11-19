// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "wirecolourentrycombobox.h"

#include <diagram.h>
#include <diagramview.h>
#include <lastusedstyle.h>
#include <projectview.h>
#include <qetdiagrameditor.h>
#include <qetproject.h>

#include <QPainter>
#include <QPixmap>

WireColourEntryComboBox::WireColourEntryComboBox(QETDiagramEditor *editor, QWidget *parent) :
	QComboBox(parent),
	m_editor(editor)
{
	setToolTip(tr("Wire colour"));
	setStatusTip(tr("Gives the selected conductors the colour, section and label of a function of the project's wire colour table",
					"status bar tip"));
	setSizeAdjustPolicy(QComboBox::AdjustToContents);
	setMinimumContentsLength(14);
	refresh();
	connect(this, QOverload<int>::of(&QComboBox::activated),
			this, &WireColourEntryComboBox::profileChosen);
}

/**
	@return a small swatch of the colours of @p profile, split in two for a
	two-colour wire
*/
QIcon WireColourEntryComboBox::swatchIcon(const WireColourEntry &profile)
{
	QPixmap pix(16, 16);
	pix.fill(Qt::transparent);
	QPainter p(&pix);
	p.setPen(QPen(QColor(0x40, 0x40, 0x40), 1));
	p.setBrush(profile.color);
	p.drawRect(0, 0, 15, 15);
	if (profile.isTwoColour()) {
		p.setPen(Qt::NoPen);
		p.setBrush(profile.color_2);
		p.drawPolygon(QPolygon({QPoint(15, 1), QPoint(15, 15), QPoint(1, 15)}));
	}
	p.end();
	return QIcon(pix);
}

QETProject *WireColourEntryComboBox::currentProject() const
{
	ProjectView *pv = m_editor ? m_editor->currentProjectView() : nullptr;
	return pv ? pv->project() : nullptr;
}

/**
	Fill the list from the table of the current project, when it is not
	the one shown. The first entry is a title, not a choice.
*/
void WireColourEntryComboBox::refresh()
{
	const QETProject *project = currentProject();
	const QList<WireColourEntry> profiles = project ? project->wireColourTable().profiles()
												: WireColourTable::defaultProfiles();
	if (count() && profiles == m_shown)
		return;

	m_shown = profiles;
	clear();
	addItem(tr("Wire colour…"));
	for (const WireColourEntry &profile : std::as_const(m_shown)) {
		QString text = profile.name;
		if (!profile.section.isEmpty())
			text += tr(" (%1 mm²)").arg(profile.section);
		addItem(swatchIcon(profile), text);
	}
	setCurrentIndex(0);
}

void WireColourEntryComboBox::showPopup()
{
	refresh();
	QComboBox::showPopup();
}

void WireColourEntryComboBox::updateEnabledState()
{
	refresh();
	ProjectView *pv = m_editor ? m_editor->currentProjectView() : nullptr;
	DiagramView *dv = pv ? pv->currentDiagram() : nullptr;
	setEnabled(dv && dv->diagram() && !dv->diagram()->isReadOnly());
}

void WireColourEntryComboBox::profileChosen(int index)
{
		//The entry 0 is the title
	if (index > 0 && index <= m_shown.count()) {
		const WireColourEntry profile = m_shown.at(index - 1);
		LastUsedStyle::setConductorColor(profile.color);
		ProjectView *pv = m_editor ? m_editor->currentProjectView() : nullptr;
		DiagramView *dv = pv ? pv->currentDiagram() : nullptr;
		if (dv)
			WireColourTable::applyToSelection(dv->diagram(), profile);
	}
	setCurrentIndex(0);
}
