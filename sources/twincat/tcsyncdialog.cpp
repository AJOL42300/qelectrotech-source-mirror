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
#include "tcsyncdialog.h"

#include "../qet.h"
#include "../qetproject.h"
#include "../undocommand/tcsynccommand.h"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QUndoStack>
#include <QVBoxLayout>

namespace {
	using Direction = TcSyncItem::Direction;

	bool reportOnly(const TcSyncItem &item)
	{
		return !item.canQetToTwincat && !item.canTwincatToQet;
	}
}

TcSyncDialog::TcSyncDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Exchange I/O names with TwinCAT"));
	QET::trackDialogGeometry(this);

	m_path = new QLineEdit(m_project ? m_project->twincatPath() : QString());
	m_path->setPlaceholderText(
		tr("TwinCAT project (.tsproj, .xti, .sln) or its folder"));
	auto *browse_file = new QPushButton(tr("Browse..."));
	auto *compare_button = new QPushButton(tr("Read and compare"));
	connect(browse_file, &QPushButton::clicked, this, &TcSyncDialog::browse);
	connect(compare_button, &QPushButton::clicked, this, &TcSyncDialog::compare);

	auto *path_layout = new QHBoxLayout;
	path_layout->addWidget(new QLabel(tr("TwinCAT project:")));
	path_layout->addWidget(m_path, 1);
	path_layout->addWidget(browse_file);
	path_layout->addWidget(compare_button);

	auto *warning = new QLabel(
		tr("TwinCAT is never started. The TwinCAT files are only changed "
		   "when you apply, after a backup copy (.bak) of each file. If "
		   "TwinCAT XAE has this project open, reload it afterwards."));
	warning->setWordWrap(true);
	m_status = new QLabel;
	m_status->setWordWrap(true);

	m_pairing = new QTableWidget(0, 2);
	m_pairing->setHorizontalHeaderLabels(
		{tr("QElectroTech card without TwinCAT box"), tr("TwinCAT box")});
	m_pairing->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_pairing->setMaximumHeight(110);
	m_pairing->hide();

	m_table = new QTableWidget(0, 5);
	m_table->setHorizontalHeaderLabels({tr("Difference"), tr("Item"),
										tr("QElectroTech"), tr("TwinCAT"),
										tr("Apply")});
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionMode(QAbstractItemView::NoSelection);

	auto *all_to_twincat = new QPushButton(tr("All: QElectroTech to TwinCAT"));
	auto *all_to_qet = new QPushButton(tr("All: TwinCAT to QElectroTech"));
	auto *none = new QPushButton(tr("None"));
	auto *preview_button = new QPushButton(tr("Preview"));
	connect(all_to_twincat, &QPushButton::clicked, this,
			[this]() {setAll(Direction::QetToTwincat);});
	connect(all_to_qet, &QPushButton::clicked, this,
			[this]() {setAll(Direction::TwincatToQet);});
	connect(none, &QPushButton::clicked, this,
			[this]() {setAll(Direction::None);});
	connect(preview_button, &QPushButton::clicked, this, &TcSyncDialog::preview);

	auto *bulk_layout = new QHBoxLayout;
	bulk_layout->addWidget(all_to_twincat);
	bulk_layout->addWidget(all_to_qet);
	bulk_layout->addWidget(none);
	bulk_layout->addStretch();
	bulk_layout->addWidget(preview_button);

	m_preview = new QPlainTextEdit;
	m_preview->setReadOnly(true);
	m_preview->setMaximumHeight(140);

	auto *buttons = new QDialogButtonBox;
	m_apply = buttons->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
	m_apply->setEnabled(false);
	buttons->addButton(QDialogButtonBox::Close);
	connect(m_apply, &QPushButton::clicked, this, &TcSyncDialog::apply);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(path_layout);
	layout->addWidget(warning);
	layout->addWidget(m_status);
	layout->addWidget(m_pairing);
	layout->addWidget(m_table, 1);
	layout->addLayout(bulk_layout);
	layout->addWidget(m_preview);
	layout->addWidget(buttons);
	resize(900, 650);
}

void TcSyncDialog::browse()
{
	const QString start = m_path->text().isEmpty()
			? QString() : QFileInfo(m_path->text()).absolutePath();
	const QString file = QFileDialog::getOpenFileName(
		this, tr("TwinCAT project"), start,
		tr("TwinCAT projects (*.tsproj *.xti *.sln);;All files (*)"));
	if (!file.isEmpty()) m_path->setText(file);
}

/// Read the TwinCAT project again and compare it with the QET project
void TcSyncDialog::compare()
{
	if (!m_project) return;
	const QString path = m_path->text().trimmed();
	if (path.isEmpty()) {
		QMessageBox::warning(this, windowTitle(),
							 tr("Choose the TwinCAT project first."));
		return;
	}
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QString error;
	TcIoTree tree;
	const bool ok = TcIoTree::load(path, tree, &error);
	if (ok) m_cards = TcQetBridge::collect(m_project);
	QApplication::restoreOverrideCursor();
	if (!ok) {
		QMessageBox::warning(
			this, windowTitle(),
			tr("The TwinCAT project cannot be read: %1").arg(error));
		return;
	}
	m_tree = tree;
	m_loaded = true;
	m_preview->clear();
	m_project->setTwincatPath(path);
	m_items = TcIoSync::compare(m_cards.cards, m_tree, m_pairs);
	m_status->setText(
		tr("%1 PLC master(s) in QElectroTech, %2 TwinCAT box(es), "
		   "%n difference(s).", nullptr, m_items.size())
		.arg(m_cards.cards.size()).arg(m_tree.boxes.size()));
	if (!m_tree.warnings.isEmpty()) {
		m_status->setText(m_status->text() + QLatin1Char('\n')
						  + m_tree.warnings.join(QLatin1Char('\n')));
	}
	fillPairing();
	fillItems();
}

/// Let the user pair the cards no TwinCAT box carries the same tag as
void TcSyncDialog::fillPairing()
{
	m_pairing->blockSignals(true);
	m_pairing->setRowCount(0);
	for (int i = 0; i < m_items.size(); ++i) {
		const TcSyncItem &item = m_items.at(i);
		if (item.kind != TcSyncItem::Kind::CardOnlyInQet) continue;
		const int row = m_pairing->rowCount();
		m_pairing->insertRow(row);
		m_pairing->setItem(row, 0, new QTableWidgetItem(item.label));
		m_pairing->item(row, 0)->setData(Qt::UserRole,
										 m_cards.cards.at(item.card).id);
		auto *combo = new QComboBox;
		combo->addItem(tr("(none)"), -1);
		for (int b = 0; b < m_tree.boxes.size(); ++b) {
			if (m_tree.boxes.at(b).channels.isEmpty()) continue;
			combo->addItem(m_tree.boxes.at(b).name, b);
		}
		const int current = combo->findData(
			m_pairs.value(m_cards.cards.at(item.card).id, -1));
		combo->setCurrentIndex(current < 0 ? 0 : current);
		connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
				this, &TcSyncDialog::pairingChanged);
		m_pairing->setCellWidget(row, 1, combo);
	}
	m_pairing->setVisible(m_pairing->rowCount() > 0);
	m_pairing->blockSignals(false);
}

void TcSyncDialog::pairingChanged()
{
	m_pairs.clear();
	for (int row = 0; row < m_pairing->rowCount(); ++row) {
		auto *combo = qobject_cast<QComboBox *>(m_pairing->cellWidget(row, 1));
		if (!combo || combo->currentData().toInt() < 0) continue;
		m_pairs.insert(m_pairing->item(row, 0)->data(Qt::UserRole).toString(),
					   combo->currentData().toInt());
	}
	m_items = TcIoSync::compare(m_cards.cards, m_tree, m_pairs);
	fillItems();
}

void TcSyncDialog::fillItems()
{
	m_table->setRowCount(0);
	for (int row = 0; row < m_items.size(); ++row) {
		const TcSyncItem &item = m_items.at(row);
		m_table->insertRow(row);
		m_table->setItem(row, 0, new QTableWidgetItem(
							 TcIoSync::kindText(item.kind)));
		m_table->setItem(row, 1, new QTableWidgetItem(item.label));
		m_table->setItem(row, 2, new QTableWidgetItem(item.qetValue));
		m_table->setItem(row, 3, new QTableWidgetItem(item.twincatValue));
		if (reportOnly(item)) {
			m_table->setItem(row, 4, new QTableWidgetItem(tr("Report only")));
			continue;
		}
		auto *combo = new QComboBox;
		combo->addItem(tr("Ignore"), static_cast<int>(Direction::None));
		if (item.canQetToTwincat) {
			combo->addItem(tr("QElectroTech to TwinCAT"),
						   static_cast<int>(Direction::QetToTwincat));
		}
		if (item.canTwincatToQet) {
			combo->addItem(tr("TwinCAT to QElectroTech"),
						   static_cast<int>(Direction::TwincatToQet));
		}
		connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
				this, [this, row](int index) {directionChanged(row, index);});
		m_table->setCellWidget(row, 4, combo);
	}
	m_apply->setEnabled(false);
}

void TcSyncDialog::directionChanged(int row, int index)
{
	auto *combo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 4));
	if (!combo || row >= m_items.size() || index < 0) return;
	m_items[row].direction =
		static_cast<Direction>(combo->itemData(index).toInt());
	m_apply->setEnabled(!currentPlan().isEmpty() || hasQetChanges());
}

void TcSyncDialog::setAll(Direction direction)
{
	for (int row = 0; row < m_items.size(); ++row) {
		auto *combo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 4));
		if (!combo) continue;
		const int index = combo->findData(static_cast<int>(direction));
		combo->setCurrentIndex(index < 0 ? 0 : index);
	}
}

TcIoWriter::Plan TcSyncDialog::currentPlan() const
{
	return TcIoSync::twincatPlan(m_items, m_cards.cards);
}

bool TcSyncDialog::hasQetChanges() const
{
	return !TcIoSync::qetUpdates(m_items, m_cards.cards, m_tree).isEmpty();
}

/// Show what would be changed on each side, without changing anything
void TcSyncDialog::preview()
{
	if (!m_loaded) return;
	QStringList lines;
	const TcIoWriter::Plan plan = currentPlan();
	if (!plan.isEmpty()) {
		const TcIoWriter::Result result = TcIoWriter::preview(m_tree, plan);
		lines << tr("TwinCAT: %1 change(s), %2 updated reference(s)")
				 .arg(result.changes.size()).arg(result.referenceUpdates);
		if (!result.ok) lines << tr("Refused: %1").arg(result.error);
		for (const auto &change : result.changes) {
			lines << QStringLiteral("  %1 -> %2").arg(change.from, change.to);
		}
		for (const QString &warning : result.warnings) {
			lines << tr("Warning: %1").arg(warning);
		}
	}
	const auto updates = TcIoSync::qetUpdates(m_items, m_cards.cards, m_tree);
	for (const TcSyncCardUpdate &update : updates) {
		lines << tr("QElectroTech: %1, %n row(s) in the I/O table", nullptr,
					update.channels.size())
				 .arg(m_cards.cards.at(update.card).tag);
	}
	if (lines.isEmpty()) lines << tr("Nothing selected.");
	m_preview->setPlainText(lines.join(QLatin1Char('\n')));
	m_apply->setEnabled(!plan.isEmpty() || !updates.isEmpty());
}

/**
	Write the TwinCAT side first (with backups), then the QET side as one
	undo step: if TwinCAT refuses, nothing at all is changed.
*/
void TcSyncDialog::apply()
{
	if (!m_loaded || !m_project) return;
	preview();
	const TcIoWriter::Plan plan = currentPlan();
	const auto updates = TcIoSync::qetUpdates(m_items, m_cards.cards, m_tree);
	if (plan.isEmpty() && updates.isEmpty()) return;

	const auto answer = QMessageBox::question(
		this, windowTitle(),
		tr("Apply the selected changes?\n\n%1\n\nThe TwinCAT files are "
		   "backed up first (.bak). If TwinCAT XAE has the project open, "
		   "reload it afterwards. The QElectroTech side can be undone.")
		.arg(m_preview->toPlainText()));
	if (answer != QMessageBox::Yes) return;

	if (!plan.isEmpty()) {
		const TcIoWriter::Result result = TcIoWriter::apply(m_tree, plan);
		if (!result.ok) {
			QMessageBox::warning(
				this, windowTitle(),
				tr("Nothing was changed: %1\n\nRead the TwinCAT project "
				   "again to continue.").arg(result.error));
			return;
		}
	}
	if (!updates.isEmpty()) {
		if (QUndoCommand *command =
				TcSyncCommand::fromUpdates(m_cards.elements, updates)) {
			m_project->undoStack()->push(command);
		}
	}
	compare();
	m_preview->setPlainText(tr("Done."));
}
