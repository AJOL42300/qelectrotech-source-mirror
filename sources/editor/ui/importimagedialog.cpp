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
#include "importimagedialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
	enum SizeMode { PixelsPerMm, WidthInCells, HeightInCells };
}

/**
	@param raster : true to ask how to trace the image too
	@param parent : parent widget
*/
ImportImageDialog::ImportImageDialog(bool raster, QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(raster ? tr("Import an image") : tr("Import a SVG drawing"));

	m_size_mode = new QComboBox(this);
	m_size_mode->addItem(tr("Element pixels per millimetre"), PixelsPerMm);
	m_size_mode->addItem(tr("Width in grid cells"), WidthInCells);
	m_size_mode->addItem(tr("Height in grid cells"), HeightInCells);
	m_size_value = new QDoubleSpinBox(this);

	auto form = new QFormLayout;
	form->addRow(tr("Size"), m_size_mode);
	form->addRow(QString(), m_size_value);

	m_threshold = new QSpinBox(this);
	m_threshold->setRange(1, 255);
	m_threshold->setValue(128);
	m_invert = new QCheckBox(tr("Trace the light pixels"), this);
	m_simplify = new QDoubleSpinBox(this);
	m_simplify->setRange(0, 10);
	m_simplify->setSingleStep(0.5);
	m_simplify->setSuffix(tr(" px"));
	m_simplify->setValue(1);
	m_despeckle = new QSpinBox(this);
	m_despeckle->setRange(0, 1000);
	m_despeckle->setSuffix(tr(" px"));
	m_despeckle->setValue(2);
	m_max_paths = new QSpinBox(this);
	m_max_paths->setRange(1, 5000);
	m_max_paths->setValue(500);
	if (raster) {
		form->addRow(tr("Threshold"), m_threshold);
		form->addRow(QString(), m_invert);
		form->addRow(tr("Simplification"), m_simplify);
		form->addRow(tr("Ignore outlines smaller than"), m_despeckle);
		form->addRow(tr("Maximum number of outlines"), m_max_paths);
	}

	auto buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);

	connect(m_size_mode, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, [this]() { updateSize(); });
	updateSize();
}

/**
	@brief ImportImageDialog::applyTo
	Set the size options chosen in @p options.
*/
void ImportImageDialog::applyTo(SvgToElmt::Options *options) const
{
	options->pixelsPerMm = 0;
	options->scale = 0;
	options->widthInCells = 0;
	options->width = 0;
	options->height = 0;
	switch (m_size_mode->currentData().toInt()) {
	case WidthInCells:
		options->widthInCells = m_size_value->value();
		break;
	case HeightInCells:
		options->height = m_size_value->value() * 10;
		break;
	default:
		options->pixelsPerMm = m_size_value->value();
	}
}

RasterVectorizer::Options ImportImageDialog::tracing() const
{
	RasterVectorizer::Options options;
	options.threshold = m_threshold->value();
	options.invert = m_invert->isChecked();
	options.simplify = m_simplify->value();
	options.despeckle = m_despeckle->value();
	options.maxPaths = m_max_paths->value();
	return options;
}

void ImportImageDialog::updateSize()
{
	if (m_size_mode->currentData().toInt() == PixelsPerMm) {
		m_size_value->setDecimals(2);
		m_size_value->setRange(0.1, 50);
		m_size_value->setSingleStep(0.5);
		m_size_value->setSuffix(tr(" px/mm"));
		m_size_value->setValue(2);
	} else {
		m_size_value->setDecimals(0);
		m_size_value->setRange(1, 60);
		m_size_value->setSingleStep(1);
		m_size_value->setSuffix(tr(" cells"));
		m_size_value->setValue(6);
	}
}
