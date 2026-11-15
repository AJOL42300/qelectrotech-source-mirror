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
#ifndef ODOOEXCHANGEDIALOG_H
#define ODOOEXCHANGEDIALOG_H

#include "odoobom.h"
#include "odooclient.h"
#include "odoomapping.h"
#include "odoosync.h"

#include <QDialog>
#include <memory>

class OdooClient;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QETProject;

/**
	@brief The OdooExchangeDialog class is the user's way to exchange the
	bill of materials of a project with Odoo: CSV files first, then the
	API on explicit request.

	Nothing is sent to Odoo before the user has seen a preview and
	confirmed; the simulation is checked by default, and then nothing is
	sent at all.
*/
class OdooExchangeDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit OdooExchangeDialog(QETProject *project, QWidget *parent = nullptr);
		~OdooExchangeDialog() override;

	private:
		void buildUi();
		OdooConnection connection() const;
		OdooMapping mapping() const;
		bool collect(QList<OdooPart> *parts, QList<OdooBom> *boms);
		bool connectToOdoo();
		void showLines(const QStringList &lines);
		void log(const QString &line);

		void exportProductsCsv();
		void exportBomCsv();
		void importCsv();
		void previewPush();
		void applyPush();
		void previewPull();
		void applyPull();
		void saveDefaultMapping();
		void updateButtons();

		QETProject *m_project;
		std::unique_ptr<OdooClient> m_client;
		OdooPushPlan m_push_plan;
		bool m_has_push_plan = false;
		QList<MaterialRecord> m_pulled;
		bool m_has_pull = false;

		QLineEdit *m_url = nullptr;
		QLineEdit *m_database = nullptr;
		QLineEdit *m_login = nullptr;
		QLineEdit *m_key = nullptr;
		QCheckBox *m_remember = nullptr;
		QComboBox *m_protocol = nullptr;
		QCheckBox *m_per_cabinet = nullptr;
		QCheckBox *m_cables = nullptr;
		QCheckBox *m_create_only = nullptr;
		QLineEdit *m_mapping_path = nullptr;
		QComboBox *m_pull_mode = nullptr;
		QCheckBox *m_simulation = nullptr;
		QListWidget *m_preview = nullptr;
		QPlainTextEdit *m_log = nullptr;
		QPushButton *m_apply_push = nullptr;
		QPushButton *m_apply_pull = nullptr;
		QLabel *m_vault_label = nullptr;
};

#endif // ODOOEXCHANGEDIALOG_H
