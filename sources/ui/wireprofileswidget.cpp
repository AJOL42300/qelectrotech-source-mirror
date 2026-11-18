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
#include "wireprofileswidget.h"

#include <QColorDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
	enum Column {NameColumn, CodeColumn, ColorColumn, StripeColumn,
				 SectionColumn, LabelColumn};

	void paintColorButton(QPushButton *button, const QColor &color)
	{
		QPixmap swatch(40, 14);
		swatch.fill(color.isValid() ? color : QColor(Qt::transparent));
		button->setIcon(QIcon(swatch));
		button->setIconSize(swatch.size());
		button->setText(color.isValid() ? QString() : WireProfilesWidget::tr("None"));
		button->setProperty("color", color);
	}
}

WireProfilesWidget::WireProfilesWidget(QWidget *parent) :
	QWidget(parent)
{
	QLabel *help = new QLabel(tr("The functions offered in the toolbar and in the menu of a "
								 "conductor. Choosing one gives the selected conductors its "
								 "colour, section and label. The colour code is the text "
								 "written on the wire (BK, RD, GNYE...). A second colour "
								 "draws the conductor striped."), this);
	help->setWordWrap(true);

	m_table = new QTableWidget(0, 6, this);
	m_table->setHorizontalHeaderLabels({tr("Function"), tr("Colour code"), tr("Colour"),
										tr("Stripe colour"), tr("Section (mm²)"),
										tr("Label")});
	m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(CodeColumn, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(SectionColumn, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(LabelColumn, QHeaderView::ResizeToContents);
	m_table->verticalHeader()->hide();
	m_table->setMinimumHeight(220);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);

	QPushButton *add_pb = new QPushButton(tr("Add"), this);
	QPushButton *remove_pb = new QPushButton(tr("Remove"), this);
	QPushButton *reset_pb = new QPushButton(tr("Restore the default table"), this);
	QHBoxLayout *buttons = new QHBoxLayout;
	buttons->addWidget(add_pb);
	buttons->addWidget(remove_pb);
	buttons->addStretch();
	buttons->addWidget(reset_pb);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(help);
	layout->addWidget(m_table);
	layout->addLayout(buttons);

	connect(add_pb, &QPushButton::clicked, this, [this]() {
		WireProfile profile;
		profile.name = tr("New function");
		addRow(profile);
		m_table->setCurrentCell(m_table->rowCount() - 1, NameColumn);
		m_table->editItem(m_table->item(m_table->rowCount() - 1, NameColumn));
	});
	connect(remove_pb, &QPushButton::clicked, this, [this]() {
		const int row = m_table->currentRow();
		if (row >= 0)
			m_table->removeRow(row);
	});
	connect(reset_pb, &QPushButton::clicked, this, [this]() {
		setProfiles(WireProfiles::defaultProfiles());
	});
}

void WireProfilesWidget::setWireProfiles(const WireProfiles &profiles)
{
	setProfiles(profiles.profiles());
}

void WireProfilesWidget::setProfiles(const QList<WireProfile> &profiles)
{
	m_table->setRowCount(0);
	for (const WireProfile &profile : profiles)
		addRow(profile);
}

void WireProfilesWidget::addRow(const WireProfile &profile)
{
	const int row = m_table->rowCount();
	m_table->insertRow(row);
	m_table->setItem(row, NameColumn, new QTableWidgetItem(profile.name));
	m_table->setItem(row, CodeColumn, new QTableWidgetItem(profile.wire_color));
	m_table->setItem(row, SectionColumn, new QTableWidgetItem(profile.section));
	m_table->setItem(row, LabelColumn, new QTableWidgetItem(profile.label));

	QPushButton *color_pb = new QPushButton(m_table);
	paintColorButton(color_pb, profile.color);
	connect(color_pb, &QPushButton::clicked, this, [this, color_pb]() {chooseColor(color_pb, false);});
	m_table->setCellWidget(row, ColorColumn, color_pb);

	QPushButton *stripe_pb = new QPushButton(m_table);
	paintColorButton(stripe_pb, profile.color_2);
	connect(stripe_pb, &QPushButton::clicked, this, [this, stripe_pb]() {chooseColor(stripe_pb, true);});
	m_table->setCellWidget(row, StripeColumn, stripe_pb);
}

	//For the stripe, a small menu offers to remove it; the dialog keeps
	//the colour when cancelled.
void WireProfilesWidget::chooseColor(QWidget *button, bool allow_none)
{
	QPushButton *color_pb = static_cast<QPushButton *>(button);
	const QColor current = color_pb->property("color").value<QColor>();
	if (allow_none && current.isValid()) {
		QMenu menu(color_pb);
		QAction *change = menu.addAction(tr("Change the colour…"));
		QAction *remove = menu.addAction(tr("No stripe"));
		const QAction *chosen = menu.exec(color_pb->mapToGlobal(QPoint(0, color_pb->height())));
		if (chosen == remove) {
			paintColorButton(color_pb, QColor());
			return;
		}
		if (chosen != change)
			return;
	}
	const QColor color = QColorDialog::getColor(current.isValid() ? current : QColor(Qt::white),
												this, tr("Choose a colour"));
	if (color.isValid())
		paintColorButton(color_pb, color);
}

/**
	@return the table as edited. A row without a function is left out, and
	so is a second row of a function already listed: a conductor keeps the
	name of its function, so two of the same name could not be told apart.
*/
WireProfiles WireProfilesWidget::wireProfiles() const
{
	QList<WireProfile> profiles;
	for (int row = 0; row < m_table->rowCount(); ++row) {
		WireProfile profile;
		profile.name = m_table->item(row, NameColumn)->text().trimmed();
		if (profile.name.isEmpty())
			continue;
		bool duplicate = false;
		for (const WireProfile &other : profiles)
			duplicate = duplicate || other.name == profile.name;
		if (duplicate)
			continue;
		profile.wire_color = m_table->item(row, CodeColumn)->text().trimmed();
		profile.color = m_table->cellWidget(row, ColorColumn)->property("color").value<QColor>();
		profile.color_2 = m_table->cellWidget(row, StripeColumn)->property("color").value<QColor>();
		profile.section = m_table->item(row, SectionColumn)->text().trimmed();
		profile.label = m_table->item(row, LabelColumn)->text().trimmed();
		profiles << profile;
	}
	WireProfiles result;
	result.setProfiles(profiles);
	return result;
}
