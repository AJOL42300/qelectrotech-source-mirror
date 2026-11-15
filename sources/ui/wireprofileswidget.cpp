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

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <TerminalStrip/terminalcatalog.h>
#include <conductorcolorcodes.h>

namespace {
	enum Column {NameColumn, ColorColumn, SectionColumn};
		//The name a row had when the widget was filled, to find renames
	constexpr int OriginalNameRole = Qt::UserRole;

	QComboBox *colorCombo(const QString &color, QWidget *parent)
	{
		QComboBox *cb = new QComboBox(parent);
		cb->setEditable(true);
		cb->addItem(QString());
		for (const auto &code : ConductorColorCodes::all()) {
			cb->addItem(ConductorColorCodes::swatch(code), code.code);
			cb->setItemData(cb->count() - 1, ConductorColorCodes::displayName(code),
							Qt::ToolTipRole);
		}
		cb->setCurrentText(color);
		return cb;
	}

	QComboBox *sectionCombo(const QString &section, QWidget *parent)
	{
		QComboBox *cb = new QComboBox(parent);
		cb->setEditable(true);
		cb->addItem(QString());
		for (const double value : TerminalCatalog::sections()) {
			cb->addItem(QLocale().toString(value, 'g', 6));
		}
		cb->setCurrentText(section);
		return cb;
	}
}

WireProfilesWidget::WireProfilesWidget(QWidget *parent) :
	QWidget(parent)
{
	QLabel *help = new QLabel(tr("Standard wires of the project. Give a conductor a "
								 "profile in its properties: it takes the colour and the "
								 "section of the profile. Changing a profile here changes "
								 "all the conductors that use it."), this);
	help->setWordWrap(true);

	m_table = new QTableWidget(0, 3, this);
	m_table->setHorizontalHeaderLabels({tr("Name"), tr("Colour"), tr("Section (mm²)")});
	m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(ColorColumn, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(SectionColumn, QHeaderView::ResizeToContents);
	m_table->verticalHeader()->hide();
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);

	QPushButton *add_pb = new QPushButton(tr("Add"), this);
	QPushButton *remove_pb = new QPushButton(tr("Remove"), this);
	QHBoxLayout *buttons = new QHBoxLayout;
	buttons->addWidget(add_pb);
	buttons->addWidget(remove_pb);
	buttons->addStretch();

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(help);
	layout->addWidget(m_table);
	layout->addLayout(buttons);

	connect(add_pb, &QPushButton::clicked, this, [this]() {
		addRow(WireProfile{tr("New wire"), QString(), QString()});
		const int row = m_table->rowCount() - 1;
			//A new profile has no former name
		m_table->item(row, NameColumn)->setData(OriginalNameRole, QString());
		m_table->setCurrentCell(row, NameColumn);
		m_table->editItem(m_table->item(row, NameColumn));
	});
	connect(remove_pb, &QPushButton::clicked, this, [this]() {
		const int row = m_table->currentRow();
		if (row >= 0) {
			m_table->removeRow(row);
		}
	});
}

void WireProfilesWidget::setWireProfiles(const WireProfiles &profiles)
{
	m_table->setRowCount(0);
	for (const WireProfile &profile : profiles.profiles()) {
		addRow(profile);
	}
}

void WireProfilesWidget::addRow(const WireProfile &profile)
{
	const int row = m_table->rowCount();
	m_table->insertRow(row);
	QTableWidgetItem *name = new QTableWidgetItem(profile.name);
	name->setData(OriginalNameRole, profile.name);
	m_table->setItem(row, NameColumn, name);
	m_table->setCellWidget(row, ColorColumn, colorCombo(profile.color, m_table));
	m_table->setCellWidget(row, SectionColumn, sectionCombo(profile.section, m_table));
}

/**
	@return the profiles in the table. A row without a name, or with a
	name already taken by a row above, is left out.
*/
WireProfiles WireProfilesWidget::wireProfiles() const
{
	QList<WireProfile> list;
	WireProfiles result;
	for (int row = 0; row < m_table->rowCount(); ++row)
	{
		WireProfile profile;
		profile.name = m_table->item(row, NameColumn)->text().trimmed();
		if (profile.name.isEmpty() || result.find(profile.name)) {
			continue;
		}
		profile.color = qobject_cast<QComboBox *>(
					m_table->cellWidget(row, ColorColumn))->currentText().trimmed();
		profile.section = qobject_cast<QComboBox *>(
					m_table->cellWidget(row, SectionColumn))->currentText().trimmed();
		list << profile;
		result.setProfiles(list);
	}
	return result;
}

/**
	@return old name -> new name, for each profile of the table whose
	name was changed
*/
QHash<QString, QString> WireProfilesWidget::renamed() const
{
	QHash<QString, QString> result;
	for (int row = 0; row < m_table->rowCount(); ++row)
	{
		const QTableWidgetItem *name = m_table->item(row, NameColumn);
		const QString before = name->data(OriginalNameRole).toString();
		const QString after = name->text().trimmed();
		if (!before.isEmpty() && !after.isEmpty() && before != after) {
			result.insert(before, after);
		}
	}
	return result;
}
