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
#include "htmldossierdialog.h"

#include "../qetproject.h"
#include "htmldossier.h"

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

namespace {
	const char *folderKey = "htmldossier/folder";
	const char *overviewKey = "htmldossier/overview";
	const char *pdfKey = "htmldossier/pdf";

	QWidget *withButton(QLineEdit *edit, QPushButton *button)
	{
		auto *w = new QWidget;
		auto *l = new QHBoxLayout(w);
		l->setContentsMargins(0, 0, 0, 0);
		l->addWidget(edit);
		l->addWidget(button);
		return w;
	}
}

/**
	@brief HtmlDossierDialog::launchDialog
	Shows the dialog for @p project
*/
void HtmlDossierDialog::launchDialog(QETProject *project, QWidget *parent)
{
	if (!project) {
		return;
	}
	HtmlDossierDialog dialog(project, parent);
	dialog.exec();
}

HtmlDossierDialog::HtmlDossierDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Export the interactive technical dossier"));
	setMinimumWidth(520);
	QSettings settings;

	m_folder = new QLineEdit(settings.value(folderKey).toString());
	m_overview = new QLineEdit(settings.value(overviewKey).toString());
	m_map = new QLineEdit;
	m_pdf = new QCheckBox(tr("Also write a PDF with bookmarks"));
	m_pdf->setChecked(settings.value(pdfKey, true).toBool());
	m_progress = new QProgressBar;
	m_progress->setVisible(false);

	auto *folder = new QPushButton(tr("Browse..."));
	auto *overview = new QPushButton(tr("Browse..."));
	auto *map = new QPushButton(tr("Browse..."));
	connect(folder, &QPushButton::clicked,
			this, &HtmlDossierDialog::browseFolder);
	connect(overview, &QPushButton::clicked,
			this, &HtmlDossierDialog::browseOverview);
	connect(map, &QPushButton::clicked, this, &HtmlDossierDialog::browseMap);

	m_overview->setPlaceholderText(
		tr("Optional: SVG picture of the whole machine"));
	m_map->setPlaceholderText(
		tr("Optional: JSON or CSV file linking functions to folios"));

	auto *form = new QFormLayout;
	form->addRow(tr("Destination folder:"), withButton(m_folder, folder));
	form->addRow(tr("Overview picture:"), withButton(m_overview, overview));
	form->addRow(tr("Function map:"), withButton(m_map, map));
	form->addRow(QString(), m_pdf);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok
									 | QDialogButtonBox::Cancel);
	connect(m_buttons, &QDialogButtonBox::accepted,
			this, &HtmlDossierDialog::run);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(m_progress);
	layout->addWidget(m_buttons);
}

void HtmlDossierDialog::browseFolder()
{
	const QString dir = QFileDialog::getExistingDirectory(
		this, tr("Destination folder"), m_folder->text());
	if (!dir.isEmpty()) {
		m_folder->setText(dir);
	}
}

void HtmlDossierDialog::browseOverview()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr("Overview picture"), m_overview->text(),
		tr("SVG pictures (*.svg)"));
	if (!file.isEmpty()) {
		m_overview->setText(file);
	}
}

void HtmlDossierDialog::browseMap()
{
	const QString file = QFileDialog::getOpenFileName(
		this, tr("Function map"), m_map->text(),
		tr("Function maps (*.json *.csv)"));
	if (!file.isEmpty()) {
		m_map->setText(file);
	}
}

void HtmlDossierDialog::run()
{
	const QString dir = m_folder->text().trimmed();
	if (dir.isEmpty()) {
		QMessageBox::warning(this, windowTitle(),
							 tr("Choose a destination folder."));
		return;
	}

	QSettings settings;
	settings.setValue(folderKey, dir);
	settings.setValue(overviewKey, m_overview->text());
	settings.setValue(pdfKey, m_pdf->isChecked());

	HtmlDossier::Options options;
	options.overviewSvg = m_overview->text().trimmed();
	options.mapFile = m_map->text().trimmed();
	options.pdf = m_pdf->isChecked();
	options.progress = [this](int done, int total, const QString &what) {
		m_progress->setRange(0, total);
		m_progress->setValue(done);
		m_progress->setFormat(what);
		qApp->processEvents();
	};

	m_buttons->setEnabled(false);
	m_progress->setVisible(true);
	const HtmlDossier::Result result =
		HtmlDossier::exportDossier(*m_project, dir, options);
	m_buttons->setEnabled(true);
	m_progress->setVisible(false);

	if (!result.ok) {
		QMessageBox::critical(this, windowTitle(), result.error);
		return;
	}
	if (!result.warnings.isEmpty()) {
		QMessageBox::warning(this, windowTitle(),
							 result.warnings.join(QLatin1Char('\n')));
	}
	QDesktopServices::openUrl(
		QUrl::fromLocalFile(QFileInfo(dir, "index.html").absoluteFilePath()));
	accept();
}
