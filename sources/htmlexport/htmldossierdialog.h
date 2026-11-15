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
#ifndef HTMLDOSSIERDIALOG_H
#define HTMLDOSSIERDIALOG_H

#include <QDialog>

class QCheckBox;
class QETProject;
class QLineEdit;
class QProgressBar;
class QDialogButtonBox;

/**
	File > Export > interactive technical dossier: asks for the folder,
	the optional overview picture and function map, then runs
	HtmlDossier::exportDossier().
*/
class HtmlDossierDialog : public QDialog
{
	Q_OBJECT

	public:
		static void launchDialog(QETProject *project, QWidget *parent);

	private:
		HtmlDossierDialog(QETProject *project, QWidget *parent);
		void browseFolder();
		void browseOverview();
		void browseMap();
		void run();

		QETProject *m_project = nullptr;
		QLineEdit *m_folder = nullptr;
		QLineEdit *m_overview = nullptr;
		QLineEdit *m_map = nullptr;
		QCheckBox *m_pdf = nullptr;
		QProgressBar *m_progress = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // HTMLDOSSIERDIALOG_H
