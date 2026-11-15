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
#ifndef TCSYNCDIALOG_H
#define TCSYNCDIALOG_H

#include "tcqetbridge.h"

#include <QDialog>
#include <QHash>
#include <QPointer>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QETProject;

/**
 * @brief The TcSyncDialog class
 * Manual, two-way exchange of I/O names between a QET project and a
 * TwinCAT project. Nothing is read or written until the user asks; every
 * difference gets its own direction, and the TwinCAT files are only
 * written after a preview and with a backup.
 */
class TcSyncDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit TcSyncDialog(QETProject *project, QWidget *parent = nullptr);

	private slots:
		void browse();
		void compare();
		void setAll(TcSyncItem::Direction direction);
		void directionChanged(int row, int index);
		void pairingChanged();
		void preview();
		void apply();

	private:
		void fillItems();
		void fillPairing();
		TcIoWriter::Plan currentPlan() const;
		bool hasQetChanges() const;

		QPointer<QETProject> m_project;
		QLineEdit *m_path = nullptr;
		QLabel *m_status = nullptr;
		QTableWidget *m_pairing = nullptr;
		QTableWidget *m_table = nullptr;
		QPlainTextEdit *m_preview = nullptr;
		QPushButton *m_apply = nullptr;
		TcIoTree m_tree;
		TcQetBridge::Cards m_cards;
		QVector<TcSyncItem> m_items;
		QHash<QString, int> m_pairs;
		bool m_loaded = false;
};

#endif // TCSYNCDIALOG_H
