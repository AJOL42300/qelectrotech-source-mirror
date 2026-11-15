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
#include "odooexchangedialog.h"

#include "../qetproject.h"
#include "odoocatalog.h"
#include "odooclient.h"
#include "odoocsv.h"
#include "odoosecretstore.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QTabWidget>
#include <QVBoxLayout>

OdooExchangeDialog::OdooExchangeDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Exchange with Odoo"));
	buildUi();
	updateButtons();
}

OdooExchangeDialog::~OdooExchangeDialog() = default;

void OdooExchangeDialog::buildUi()
{
	auto *tabs = new QTabWidget(this);

		//Connection
	auto *connection_page = new QWidget(this);
	auto *form = new QFormLayout(connection_page);
	OdooConnection saved;
	saved.loadSettings();
	m_url = new QLineEdit(saved.url.toString(), this);
	m_database = new QLineEdit(saved.database, this);
	m_login = new QLineEdit(saved.login, this);
	m_key = new QLineEdit(this);
	m_key->setEchoMode(QLineEdit::Password);
	m_key->setPlaceholderText(tr("API key"));
	m_remember = new QCheckBox(tr("Remember the API key in the system vault"), this);
	OdooSecretStore *vault = OdooSecretStore::system();
	m_remember->setEnabled(vault->isPersistent());
	m_vault_label = new QLabel(vault->isPersistent()
			? tr("The key is kept by %1, never in the project nor in a file.").arg(vault->name())
			: tr("This system has no vault: the key is asked again each time."), this);
	m_vault_label->setWordWrap(true);
	m_protocol = new QComboBox(this);
	m_protocol->addItem(tr("Automatic"), static_cast<int>(OdooProtocol::Auto));
	m_protocol->addItem(QStringLiteral("JSON-2 (Odoo 19+)"), static_cast<int>(OdooProtocol::Json2));
	m_protocol->addItem(QStringLiteral("JSON-RPC (Odoo 17, 18)"), static_cast<int>(OdooProtocol::JsonRpc));
	m_protocol->setCurrentIndex(m_protocol->findData(static_cast<int>(saved.protocol)));
	auto *test = new QPushButton(tr("Test the connection"), this);
	connect(test, &QPushButton::clicked, this, [this]() {
		if (connectToOdoo()) {
			QMessageBox::information(this, windowTitle(),
									 tr("Connected (%1, version %2).")
									 .arg(OdooConnection::protocolName(m_client->protocol()),
										  m_client->serverVersion().isEmpty() ? QStringLiteral("?") : m_client->serverVersion()));
		}
	});
	form->addRow(tr("Address"), m_url);
	form->addRow(tr("Database"), m_database);
	form->addRow(tr("User (login or e-mail)"), m_login);
	form->addRow(tr("API key"), m_key);
	form->addRow(QString(), m_remember);
	form->addRow(QString(), m_vault_label);
	form->addRow(tr("Protocol"), m_protocol);
	form->addRow(QString(), test);
	tabs->addTab(connection_page, tr("Connection"));

		//Content
	auto *content_page = new QWidget(this);
	auto *content = new QVBoxLayout(content_page);
	m_per_cabinet = new QCheckBox(tr("One bill of materials per cabinet"), this);
	m_cables = new QCheckBox(tr("Add the cables (length per reference)"), this);
	m_create_only = new QCheckBox(tr("Only create the missing products, do not update the others"), this);
	m_create_only->setChecked(true);
	m_mapping_path = new QLineEdit(this);
	m_mapping_path->setPlaceholderText(tr("Mapping file (JSON), empty for the default one"));
	auto *browse = new QPushButton(tr("Browse..."), this);
	connect(browse, &QPushButton::clicked, this, [this]() {
		const QString path = QFileDialog::getOpenFileName(this, tr("Mapping file"),
														  m_mapping_path->text(), tr("JSON (*.json)"));
		if (!path.isEmpty()) {
			m_mapping_path->setText(path);
		}
	});
	auto *save_mapping = new QPushButton(tr("Save the default mapping..."), this);
	connect(save_mapping, &QPushButton::clicked, this, &OdooExchangeDialog::saveDefaultMapping);
	auto *mapping_line = new QHBoxLayout();
	mapping_line->addWidget(m_mapping_path);
	mapping_line->addWidget(browse);
	content->addWidget(m_per_cabinet);
	content->addWidget(m_cables);
	content->addLayout(mapping_line);
	content->addWidget(save_mapping, 0, Qt::AlignLeft);
	content->addStretch();
	tabs->addTab(content_page, tr("Content"));

		//CSV
	auto *csv_page = new QWidget(this);
	auto *csv = new QVBoxLayout(csv_page);
	auto *products_csv = new QPushButton(tr("Export the products (CSV for Odoo)..."), this);
	auto *bom_csv = new QPushButton(tr("Export the bills of materials (CSV for Odoo)..."), this);
	auto *import_csv = new QPushButton(tr("Import a CSV exported by Odoo into the catalogue..."), this);
	connect(products_csv, &QPushButton::clicked, this, &OdooExchangeDialog::exportProductsCsv);
	connect(bom_csv, &QPushButton::clicked, this, &OdooExchangeDialog::exportBomCsv);
	connect(import_csv, &QPushButton::clicked, this, &OdooExchangeDialog::importCsv);
	csv->addWidget(products_csv);
	csv->addWidget(bom_csv);
	csv->addWidget(import_csv);
	csv->addStretch();
	tabs->addTab(csv_page, tr("CSV files"));

		//Odoo
	auto *odoo_page = new QWidget(this);
	auto *odoo = new QVBoxLayout(odoo_page);
	m_simulation = new QCheckBox(tr("Simulation: show what would be done, change nothing"), this);
	m_simulation->setChecked(true);
	m_pull_mode = new QComboBox(this);
	m_pull_mode->addItem(tr("Catalogue: fill the empty cells only"), static_cast<int>(OdooCatalog::Mode::FillEmpty));
	m_pull_mode->addItem(tr("Catalogue: overwrite with the values of Odoo"), static_cast<int>(OdooCatalog::Mode::Overwrite));
	auto *push_preview = new QPushButton(tr("Preview QElectroTech -> Odoo"), this);
	m_apply_push = new QPushButton(tr("Apply to Odoo"), this);
	auto *pull_preview = new QPushButton(tr("Preview Odoo -> QElectroTech"), this);
	m_apply_pull = new QPushButton(tr("Apply to the catalogue"), this);
	connect(push_preview, &QPushButton::clicked, this, &OdooExchangeDialog::previewPush);
	connect(m_apply_push, &QPushButton::clicked, this, &OdooExchangeDialog::applyPush);
	connect(pull_preview, &QPushButton::clicked, this, &OdooExchangeDialog::previewPull);
	connect(m_apply_pull, &QPushButton::clicked, this, &OdooExchangeDialog::applyPull);
	connect(m_simulation, &QCheckBox::toggled, this, &OdooExchangeDialog::updateButtons);
	m_preview = new QListWidget(this);
	odoo->addWidget(m_simulation);
	odoo->addWidget(m_create_only);
	odoo->addWidget(m_pull_mode);
	auto *buttons_push = new QHBoxLayout();
	buttons_push->addWidget(push_preview);
	buttons_push->addWidget(m_apply_push);
	auto *buttons_pull = new QHBoxLayout();
	buttons_pull->addWidget(pull_preview);
	buttons_pull->addWidget(m_apply_pull);
	odoo->addLayout(buttons_push);
	odoo->addLayout(buttons_pull);
	odoo->addWidget(m_preview, 1);
	tabs->addTab(odoo_page, tr("Odoo"));

	m_log = new QPlainTextEdit(this);
	m_log->setReadOnly(true);
	m_log->setMaximumHeight(90);
	auto *close = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *main = new QVBoxLayout(this);
	main->addWidget(tabs, 1);
	main->addWidget(m_log);
	main->addWidget(close);
	resize(640, 560);
}

void OdooExchangeDialog::updateButtons()
{
	const QString push = m_simulation->isChecked()
			? tr("Simulate the changes in Odoo") : tr("Apply to Odoo");
	const QString pull = m_simulation->isChecked()
			? tr("Simulate the update of the catalogue") : tr("Apply to the catalogue");
	m_apply_push->setText(push);
	m_apply_pull->setText(pull);
	m_apply_push->setEnabled(m_has_push_plan);
	m_apply_pull->setEnabled(m_has_pull);
}

OdooConnection OdooExchangeDialog::connection() const
{
	OdooConnection c;
	c.url = QUrl(m_url->text().trimmed());
	c.database = m_database->text().trimmed();
	c.login = m_login->text().trimmed();
	c.api_key = m_key->text();
	c.protocol = static_cast<OdooProtocol>(m_protocol->currentData().toInt());
	return c;
}

OdooMapping OdooExchangeDialog::mapping() const
{
	OdooMapping m = OdooMapping::defaults();
	const QString path = m_mapping_path->text().trimmed();
	if (!path.isEmpty())
	{
		QString error;
		if (!OdooMapping::load(path, &m, &error))
		{
			QMessageBox::warning(const_cast<OdooExchangeDialog *>(this), windowTitle(),
								 tr("The mapping file cannot be used (%1): the default mapping is used.").arg(error));
			return OdooMapping::defaults();
		}
	}
	return m;
}

void OdooExchangeDialog::log(const QString &line)
{
	m_log->appendPlainText(line);
}

void OdooExchangeDialog::showLines(const QStringList &lines)
{
	m_preview->clear();
	m_preview->addItems(lines);
}

void OdooExchangeDialog::saveDefaultMapping()
{
	const QString path = QFileDialog::getSaveFileName(this, tr("Save the default mapping"),
													  QStringLiteral("odoo_mapping.json"),
													  tr("JSON (*.json)"));
	if (path.isEmpty()) {
		return;
	}
	QString error;
	if (OdooMapping::defaults().save(path, &error)) {
		m_mapping_path->setText(path);
	} else {
		QMessageBox::warning(this, windowTitle(), error);
	}
}

/**
	@brief OdooExchangeDialog::collect
	Read the parts of the project and group them into bills of materials.
*/
bool OdooExchangeDialog::collect(QList<OdooPart> *parts, QList<OdooBom> *boms)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_project->dataBase()->updateDB();
	QString rejection;
	auto query = m_project->dataBase()->newQuery(OdooBomBuilder::partsQuery(true), &rejection);
	if (!rejection.isEmpty() || !query.exec())
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, windowTitle(),
							 rejection.isEmpty() ? query.lastError().text() : rejection);
		return false;
	}
	*parts = OdooBomBuilder::readParts(query);
	auto unreferenced = m_project->dataBase()->newQuery(OdooBomBuilder::unreferencedQuery(true));
	if (unreferenced.exec())
	{
		const int left_out = OdooBomBuilder::readCount(unreferenced);
		if (left_out > 0) {
			log(tr("%n component(s) without manufacturer reference are not exchanged.", nullptr, left_out));
		}
	}
	if (m_cables->isChecked()) {
		parts->append(OdooBomBuilder::cableParts(m_project->toXml()));
	}
	QApplication::restoreOverrideCursor();

	OdooBomOptions options;
	options.per_cabinet = m_per_cabinet->isChecked();
	options.project_name = m_project->title().isEmpty()
			? QFileInfo(m_project->filePath()).completeBaseName() : m_project->title();
	*boms = OdooBomBuilder::buildBoms(*parts, options);
	if (parts->isEmpty())
	{
		QMessageBox::information(this, windowTitle(),
								 tr("No part with a manufacturer reference was found in this project."));
		return false;
	}
	return true;
}

void OdooExchangeDialog::exportProductsCsv()
{
	QList<OdooPart> parts;
	QList<OdooBom> boms;
	if (!collect(&parts, &boms)) {
		return;
	}
	const QString path = QFileDialog::getSaveFileName(this, tr("Export the products"),
													  QStringLiteral("odoo_products.csv"),
													  tr("CSV (*.csv)"));
	if (path.isEmpty()) {
		return;
	}
	const OdooMapping m = mapping();
	QString error;
	if (OdooCsv::writeFile(path, OdooCsv::productsCsv(OdooBomBuilder::productsOfParts(parts), boms, m), &error)) {
		log(tr("Products written to %1").arg(path));
	} else {
		QMessageBox::warning(this, windowTitle(), error);
	}
}

void OdooExchangeDialog::exportBomCsv()
{
	QList<OdooPart> parts;
	QList<OdooBom> boms;
	if (!collect(&parts, &boms)) {
		return;
	}
	const QString path = QFileDialog::getSaveFileName(this, tr("Export the bills of materials"),
													  QStringLiteral("odoo_bom.csv"),
													  tr("CSV (*.csv)"));
	if (path.isEmpty()) {
		return;
	}
	QString error;
	if (OdooCsv::writeFile(path, OdooCsv::bomCsv(boms, mapping()), &error)) {
		log(tr("Bills of materials written to %1").arg(path));
	} else {
		QMessageBox::warning(this, windowTitle(), error);
	}
}

void OdooExchangeDialog::importCsv()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("CSV exported by Odoo"),
													  QString(), tr("CSV (*.csv)"));
	if (path.isEmpty()) {
		return;
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, windowTitle(), file.errorString());
		return;
	}
	OdooImportResult result;
	QString error;
	if (!OdooCsv::importProducts(file.readAll(), mapping(), &result, &error))
	{
		QMessageBox::warning(this, windowTitle(), error);
		return;
	}
	m_pulled = result.records;
	m_has_pull = !m_pulled.isEmpty();
	QStringList lines = result.warnings;
	MaterialListData data;
	const QString catalogue = MaterialList::configuredPath();
	QString load_error;
	if (catalogue.isEmpty() || !MaterialList::load(catalogue, &data, &load_error))
	{
		QMessageBox::warning(this, windowTitle(),
							 tr("The user catalogue cannot be read: choose it first in the settings of QElectroTech. %1").arg(load_error));
		m_has_pull = false;
	}
	else
	{
		const auto changes = OdooCatalog::merge(&data, m_pulled,
												static_cast<OdooCatalog::Mode>(m_pull_mode->currentData().toInt()), false);
		for (const auto &change : changes) {
			lines << (change.is_new
					  ? tr("New article %1").arg(change.reference)
					  : QStringLiteral("%1: %2 [%3 -> %4]").arg(change.reference, change.column, change.old_value, change.new_value));
		}
	}
	showLines(lines);
	updateButtons();
}

bool OdooExchangeDialog::connectToOdoo()
{
	OdooConnection c = connection();
	OdooSecretStore *vault = OdooSecretStore::system();
	const QString account = OdooSecretStore::accountFor(c.url, c.login);
	if (c.api_key.isEmpty() && vault->isPersistent()) {
		c.api_key = vault->read(account);
	}
	m_client = std::make_unique<OdooClient>(c);
	connect(m_client.get(), &OdooClient::logged, this, &OdooExchangeDialog::log);
	QString error;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const bool ok = m_client->connectToServer(&error);
	QApplication::restoreOverrideCursor();
	if (!ok)
	{
		QMessageBox::warning(this, windowTitle(), error);
		m_client.reset();
		return false;
	}
	c.saveSettings();
	if (m_remember->isChecked() && vault->isPersistent() && !m_key->text().isEmpty())
	{
		QString store_error;
		if (!vault->store(account, m_key->text(), &store_error)) {
			log(tr("The key could not be saved: %1").arg(store_error));
		}
		m_key->clear();
	}
	return true;
}

void OdooExchangeDialog::previewPush()
{
	m_has_push_plan = false;
	updateButtons();
	QList<OdooPart> parts;
	QList<OdooBom> boms;
	if (!collect(&parts, &boms) || !connectToOdoo()) {
		return;
	}
	OdooPushOptions options;
	options.create_only = m_create_only->isChecked();
	QString error;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	OdooSync sync(m_client.get(), mapping());
	const bool ok = sync.planPush(OdooBomBuilder::productsOfParts(parts), boms, options, &m_push_plan, &error);
	QApplication::restoreOverrideCursor();
	if (!ok)
	{
		QMessageBox::warning(this, windowTitle(), error);
		return;
	}
	QStringList lines = m_push_plan.describe();
	if (m_push_plan.writeCalls() == 0) {
		lines.prepend(tr("Nothing to change in Odoo."));
	}
	showLines(lines);
	m_has_push_plan = m_push_plan.writeCalls() > 0;
	updateButtons();
}

void OdooExchangeDialog::applyPush()
{
	if (!m_has_push_plan || !m_client) {
		return;
	}
	const bool simulate = m_simulation->isChecked();
	if (!simulate)
	{
		const auto answer = QMessageBox::question(
					this, windowTitle(),
					tr("Create %1 and update %2 records in %3? Nothing is ever deleted.")
					.arg(m_push_plan.count(OdooAction::Create))
					.arg(m_push_plan.count(OdooAction::Update))
					.arg(m_url->text()));
		if (answer != QMessageBox::Yes) {
			return;
		}
	}
	QApplication::setOverrideCursor(Qt::WaitCursor);
	OdooSync sync(m_client.get(), mapping());
	const OdooPushResult result = sync.applyPush(m_push_plan, simulate);
	QApplication::restoreOverrideCursor();
	showLines(result.messages);
	if (!simulate)
	{
		m_has_push_plan = false;
		if (!result.ok) {
			QMessageBox::warning(this, windowTitle(), result.messages.value(result.messages.size() - 1));
		}
	}
	updateButtons();
}

void OdooExchangeDialog::previewPull()
{
	m_has_pull = false;
	updateButtons();
	QList<OdooPart> parts;
	QList<OdooBom> boms;
	if (!collect(&parts, &boms) || !connectToOdoo()) {
		return;
	}
	QStringList references;
	for (const OdooPart &part : OdooBomBuilder::productsOfParts(parts)) {
		references << part.reference;
	}
	MaterialListData data;
	QString error;
	const QString catalogue = MaterialList::configuredPath();
	if (catalogue.isEmpty() || !MaterialList::load(catalogue, &data, &error))
	{
		QMessageBox::warning(this, windowTitle(),
							 tr("The user catalogue cannot be read: choose it first in the settings of QElectroTech. %1").arg(error));
		return;
	}
	OdooPullResult pulled;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	OdooSync sync(m_client.get(), mapping());
	const bool ok = sync.fetchProducts(references, 0, &pulled, &error);
	QApplication::restoreOverrideCursor();
	if (!ok)
	{
		QMessageBox::warning(this, windowTitle(), error);
		return;
	}
	QStringList lines = pulled.warnings;
	for (const QString &missing : pulled.not_found) {
		lines << tr("Not in Odoo: %1").arg(missing);
	}
	const auto changes = OdooCatalog::merge(&data, pulled.records,
											static_cast<OdooCatalog::Mode>(m_pull_mode->currentData().toInt()), false);
	for (const auto &change : changes) {
		lines << (change.is_new
				  ? tr("New article %1").arg(change.reference)
				  : QStringLiteral("%1: %2 [%3 -> %4]").arg(change.reference, change.column, change.old_value, change.new_value));
	}
	if (changes.isEmpty()) {
		lines.prepend(tr("The catalogue is already up to date."));
	}
	showLines(lines);
	m_pulled = pulled.records;
	m_has_pull = !changes.isEmpty();
	updateButtons();
}

void OdooExchangeDialog::applyPull()
{
	if (!m_has_pull) {
		return;
	}
	if (m_simulation->isChecked())
	{
		log(tr("Simulation: the catalogue was not changed."));
		return;
	}
	MaterialListData data;
	QString error;
	const QString catalogue = MaterialList::configuredPath();
	if (catalogue.isEmpty() || !MaterialList::load(catalogue, &data, &error))
	{
		QMessageBox::warning(this, windowTitle(), error);
		return;
	}
	const auto mode = static_cast<OdooCatalog::Mode>(m_pull_mode->currentData().toInt());
	const int count = OdooCatalog::merge(&data, m_pulled, mode, false).size();
	if (QMessageBox::question(this, windowTitle(),
							  tr("Change %n cell(s) of the catalogue? A copy of the file is kept (.bak).", nullptr, count)) != QMessageBox::Yes) {
		return;
	}
	OdooCatalog::merge(&data, m_pulled, mode, true);
	if (OdooCatalog::saveWithBackup(catalogue, data, &error))
	{
		log(tr("Catalogue updated: %1").arg(catalogue));
		m_has_pull = false;
		updateButtons();
	} else {
		QMessageBox::warning(this, windowTitle(), error);
	}
}
