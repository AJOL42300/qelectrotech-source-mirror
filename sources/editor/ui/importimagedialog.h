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
#ifndef IMPORTIMAGEDIALOG_H
#define IMPORTIMAGEDIALOG_H

#include <svgtoelmt.h>

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

/**
	@brief The ImportImageDialog class
	Asks how to turn a SVG drawing or a raster image into an element:
	the size (pixels per millimetre, or a width or height in grid cells)
	and, for a raster image, how to trace it.
*/
class ImportImageDialog : public QDialog
{
	Q_OBJECT

	public:
		ImportImageDialog(bool raster, QWidget *parent = nullptr);

		void applyTo(SvgToElmt::Options *options) const;
		RasterVectorizer::Options tracing() const;

	private:
		void updateSize();

		QComboBox *m_size_mode = nullptr;
		QDoubleSpinBox *m_size_value = nullptr;
		QSpinBox *m_threshold = nullptr;
		QCheckBox *m_invert = nullptr;
		QDoubleSpinBox *m_simplify = nullptr;
		QSpinBox *m_despeckle = nullptr;
		QSpinBox *m_max_paths = nullptr;
};

#endif // IMPORTIMAGEDIALOG_H
