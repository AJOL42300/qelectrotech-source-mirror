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
#include "qetapptest.h"
#include "diagramcontent.h"
#include "imagestorage.h"
#include "qetgraphicsitem/diagramimageitem.h"

#include <QDomDocument>
#include <QPainter>
#include <QSettings>
#include <QStyleOptionGraphicsItem>

/**
	Stretch, crop, opacity, rotation by 90 degrees and storage format
	(PNG or JPEG) of the pictures of a folio: each one is undoable,
	survives a save and a load, and an old file without the new
	attributes still loads as it did.
*/
class tst_imagestretchcrop : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;

	static QImage opaqueImage(int width = 60, int height = 40)
	{
		QImage image(width, height, QImage::Format_ARGB32);
		for (int y = 0 ; y < height ; ++y)
			for (int x = 0 ; x < width ; ++x)
				image.setPixel(x, y, qRgb(x * 4 % 256, y * 6 % 256, 90));
		return image;
	}

	static QImage translucentImage()
	{
		QImage image = opaqueImage();
		image.setPixel(3, 3, qRgba(10, 20, 30, 100));
		return image;
	}

	DiagramImageItem *addPicture(const QImage &image)
	{
		auto *picture = new DiagramImageItem(QPixmap::fromImage(image));
		m_diagram->addItem(picture);
		picture->setSelected(true);
		return picture;
	}

	void removePictures()
	{
		m_diagram->clearSelection();
		for (QGraphicsItem *item : m_diagram->items())
			if (item->type() == DiagramImageItem::Type)
				delete item;
	}

	/// The <image> elements of @p document, in file order
	static QList<QDomElement> imageElements(const QDomDocument &document)
	{
		QList<QDomElement> result;
		const QDomElement images = document.documentElement()
				.firstChildElement(QStringLiteral("images"));
		for (QDomElement e = images.firstChildElement(QStringLiteral("image"));
			 !e.isNull();
			 e = e.nextSiblingElement(QStringLiteral("image")))
			result << e;
		return result;
	}

	/// The picture file stored in the first <image> element
	static QByteArray storedBytes(const QDomDocument &document)
	{
		const QDomElement e = imageElements(document).value(0);
		return QByteArray::fromBase64(
					e.firstChild().toText().data().toLatin1());
	}

	/// Saves then loads the selected pictures; returns the loaded one
	DiagramImageItem *roundTrip(QDomDocument *saved = nullptr)
	{
		QDomDocument document = m_diagram->toXml(false);
		if (saved)
			*saved = document;
		removePictures();
		DiagramContent content;
		if (!m_diagram->fromXml(document, QPointF(), false, &content)
				|| content.m_images.size() != 1)
			return nullptr;
		return *content.m_images.begin();
	}

	struct SettingsGuard
	{
		QSettings settings;
		QVariant convert = settings.value(ImageStorage::convertKey);
		QVariant quality = settings.value(ImageStorage::qualityKey);
		~SettingsGuard()
		{
			restore(ImageStorage::convertKey, convert);
			restore(ImageStorage::qualityKey, quality);
		}
		void restore(const char *key, const QVariant &value)
		{
			if (value.isValid())
				settings.setValue(key, value);
			else
				settings.remove(key);
		}
	};

private slots:
	void initTestCase()
	{
		QVERIFY(QetAppTest::editor());
		m_diagram = QetAppTest::diagram();
		QVERIFY(m_diagram);
	}

	void cleanup()
	{
		removePictures();
		m_diagram->undoStack().clear();
	}

	void stretchIsIndependentAndSurvivesSaving()
	{
		DiagramImageItem *picture = addPicture(opaqueImage());
		picture->setScaleFactorX(2.0);
		picture->setScaleFactorY(0.5);

		const DiagramImageItem *loaded = roundTrip();
		QVERIFY(loaded);
		QCOMPARE(loaded->scaleFactorX(), 2.0);
		QCOMPARE(loaded->scaleFactorY(), 0.5);
		QCOMPARE(loaded->pixmap().size(), QSize(60, 40));
	}

	void cropKeepsTheOriginalAndIsUndoable()
	{
		DiagramImageItem *picture = addPicture(opaqueImage());
		QVERIFY(picture->applyCrop(QRect(10, 5, 20, 10)));
		QCOMPARE(picture->pixmap().size(), QSize(20, 10));
		QCOMPARE(picture->imageSource().base.size(), QSize(60, 40));

		m_diagram->undoStack().undo();
		QCOMPARE(picture->pixmap().size(), QSize(60, 40));
		m_diagram->undoStack().redo();
		QCOMPARE(picture->pixmap().size(), QSize(20, 10));

		// the crop can still be widened: the original was kept
		QVERIFY(picture->applyCrop(QRect(0, 0, 40, 30)));
		QCOMPARE(picture->pixmap().size(), QSize(40, 30));
	}

	void cropSurvivesSaving()
	{
		DiagramImageItem *picture = addPicture(opaqueImage());
		QVERIFY(picture->applyCrop(QRect(10, 5, 20, 10)));

		const DiagramImageItem *loaded = roundTrip();
		QVERIFY(loaded);
		QCOMPARE(loaded->cropRect(), QRect(10, 5, 20, 10));
		QCOMPARE(loaded->pixmap().size(), QSize(20, 10));
		QCOMPARE(loaded->imageSource().base.size(), QSize(60, 40));
	}

	void rotationBy90IsUndoable()
	{
		DiagramImageItem *picture = addPicture(opaqueImage());
		picture->setScaleFactorX(2.0);
		picture->rotateBy(90.0);
		QCOMPARE(picture->rotationAngle(), 90.0);
		QCOMPARE(picture->scaleFactorX(), 2.0);
		picture->rotateBy(-90.0);
		QCOMPARE(picture->rotationAngle(), 0.0);
		m_diagram->undoStack().undo();
		QCOMPARE(picture->rotationAngle(), 90.0);
		m_diagram->undoStack().undo();
		QCOMPARE(picture->rotationAngle(), 0.0);
	}

	void opacitySurvivesSavingAndIsAbsentAtFull()
	{
		DiagramImageItem *picture = addPicture(opaqueImage());
		QDomDocument document;
		DiagramImageItem *loaded = roundTrip(&document);
		QVERIFY(loaded);
		QVERIFY(!imageElements(document).value(0)
				.hasAttribute(QStringLiteral("opacity")));
		QCOMPARE(loaded->pictureOpacity(), 1.0);
		Q_UNUSED(picture);

		loaded->setSelected(true);
		loaded->setPictureOpacity(0.4);
		DiagramImageItem *again = roundTrip(&document);
		QVERIFY(again);
		QVERIFY(imageElements(document).value(0)
				.hasAttribute(QStringLiteral("opacity")));
		QVERIFY(qAbs(again->pictureOpacity() - 0.4) < 1e-6);
	}

	void opacityReachesThePainter()
	{
		QImage black(20, 20, QImage::Format_ARGB32);
		black.fill(Qt::black);
		DiagramImageItem *picture = addPicture(black);
		picture->setSelected(false);

		auto center = [this, picture]() {
			QImage target(40, 40, QImage::Format_ARGB32);
			target.fill(Qt::white);
			QPainter painter(&target);
			const QRectF area = picture->sceneBoundingRect();
			m_diagram->render(&painter, QRectF(0, 0, 40, 40),
							  area.adjusted(-10, -10, 10, 10));
			painter.end();
			return qRed(target.pixel(23, 21));
		};
		QVERIFY(center() < 20);
		picture->setPictureOpacity(0.5);
		const int half = center();
		QVERIFY2(half > 100 && half < 160, qPrintable(QString::number(half)));
	}

	void oldFileWithoutNewAttributesLoadsUnchanged()
	{
		const QImage image = opaqueImage();
		DiagramImageItem *picture = addPicture(image);
		picture->setScaleFactorX(1.5);
		QDomDocument document = m_diagram->toXml(false);
		removePictures();

		QDomElement e = imageElements(document).value(0);
		QVERIFY(!e.hasAttribute(QStringLiteral("opacity")));
		QVERIFY(ImageStorage::isPng(storedBytes(document)));
		QVERIFY(!e.hasAttribute(QStringLiteral("keep_alpha")));

		DiagramContent content;
		QVERIFY(m_diagram->fromXml(document, QPointF(), false, &content));
		QCOMPARE(content.m_images.size(), 1);
		const DiagramImageItem *loaded 		= *content.m_images.begin();
		QCOMPARE(loaded->pictureOpacity(), 1.0);
		QVERIFY(loaded->keepAlpha());
		QCOMPARE(loaded->scaleFactorX(), 1.5);
		QCOMPARE(loaded->pixmap().toImage()
				 .convertToFormat(QImage::Format_ARGB32), image);
	}

	void transparencyKeepsPng()
	{
		SettingsGuard guard;
		guard.settings.setValue(ImageStorage::convertKey, true);
		DiagramImageItem *picture = addPicture(translucentImage());
		picture->setStoredFormat(true);
		QDomDocument document;
		DiagramImageItem *loaded = roundTrip(&document);
		QVERIFY(loaded);
		QVERIFY(ImageStorage::isPng(storedBytes(document)));
		QCOMPARE(qAlpha(loaded->pixmap().toImage()
						.convertToFormat(QImage::Format_ARGB32).pixel(3, 3)),
				 100);
	}

	void opaquePictureBecomesJpegWhenConversionIsOn()
	{
		SettingsGuard guard;
		guard.settings.setValue(ImageStorage::convertKey, true);
		guard.settings.setValue(ImageStorage::qualityKey, 85);
		DiagramImageItem *picture = addPicture(opaqueImage());
		picture->setStoredFormat(false);
		QDomDocument document;
		DiagramImageItem *loaded = roundTrip(&document);
		QVERIFY(loaded);
		QVERIFY(ImageStorage::isJpeg(storedBytes(document)));
		QVERIFY(!loaded->keepAlpha());
		QCOMPARE(loaded->pixmap().size(), QSize(60, 40));
	}

	void conversionOffKeepsPng()
	{
		SettingsGuard guard;
		guard.settings.setValue(ImageStorage::convertKey, false);
		DiagramImageItem *picture = addPicture(opaqueImage());
		picture->setStoredFormat(false);
		QDomDocument document;
		QVERIFY(roundTrip(&document));
		QVERIFY(ImageStorage::isPng(storedBytes(document)));
	}

	void jpegIsNotRecompressed()
	{
		SettingsGuard guard;
		guard.settings.setValue(ImageStorage::convertKey, true);
		const QByteArray jpeg = ImageStorage::encode(
					opaqueImage(), false, true, 60);
		QVERIFY(ImageStorage::isJpeg(jpeg));
		QImage decoded;
		QVERIFY(decoded.loadFromData(jpeg));

		DiagramImageItem *picture = addPicture(decoded);
		picture->setStoredFormat(false, jpeg);
		QDomDocument document;
		QVERIFY(roundTrip(&document));
		QCOMPARE(storedBytes(document), jpeg);
	}

	void transparentColorAndToleranceSurviveSavingAsPng()
	{
		SettingsGuard guard;
		guard.settings.setValue(ImageStorage::convertKey, true);
		DiagramImageItem *picture = addPicture(opaqueImage());
		picture->setStoredFormat(false);
		DiagramImageItem::ImageSource source = picture->imageSource();
		source.colors = {{QColor(0, 0, 90), 25}};
		picture->setImageSource(source);
		QVERIFY(picture->hasTransparentColors());

		QDomDocument document;
		DiagramImageItem *loaded = roundTrip(&document);
		QVERIFY(loaded);
		QVERIFY(ImageStorage::isPng(storedBytes(document)));
		const auto colors = loaded->imageSource().colors;
		QCOMPARE(colors.size(), 1);
		QCOMPARE(colors.first().color, QColor(0, 0, 90));
		QCOMPARE(colors.first().tolerance, 25);
		// the pixels of that colour are transparent, the original is kept
		const QImage shown = loaded->pixmap().toImage()
				.convertToFormat(QImage::Format_ARGB32);
		QCOMPARE(qAlpha(shown.pixel(0, 0)), 0);
		QCOMPARE(qAlpha(shown.pixel(50, 30)), 255);
		QVERIFY(loaded->imageSource().base.toImage()
				.convertToFormat(QImage::Format_ARGB32)
				.pixel(0, 0) == qRgb(0, 0, 90));
	}

	void storageHelpers()
	{
		QVERIFY(!ImageStorage::hasTranslucentPixels(opaqueImage()));
		QVERIFY(ImageStorage::hasTranslucentPixels(translucentImage()));

		const QByteArray png = ImageStorage::encode(
					translucentImage(), true, true, 85);
		QVERIFY(ImageStorage::isPng(png));
		const QByteArray jpeg = ImageStorage::encode(
					translucentImage(), false, true, 85);
		QVERIFY(ImageStorage::isJpeg(jpeg));
		// a transparent pixel is flattened on white, not on black
		QImage flat;
		QVERIFY(flat.loadFromData(
					ImageStorage::encode(
						[]() {
							QImage i(8, 8, QImage::Format_ARGB32);
							i.fill(Qt::transparent);
							return i;
						}(), false, true, 95)));
		QVERIFY(qRed(flat.pixel(4, 4)) > 240);
	}
};

QET_APP_TEST_MAIN(tst_imagestretchcrop)
#include "tst_imagestretchcrop.moc"
