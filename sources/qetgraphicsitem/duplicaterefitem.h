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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DUPLICATEREFITEM_H
#define DUPLICATEREFITEM_H

#include <QGraphicsObject>
#include <QList>
#include <QPointer>
#include <QRectF>
#include <QString>

class Element;

/**
	@brief The references of a symbol to the other representations of its
	component, drawn under it (or on its right).

	Child of the symbol, made by ComponentReferenceIndex only for a symbol
	which has other representations; it hides itself when it has none.
	A click on a reference goes to that representation: its folio is shown
	and the symbol is selected, as for a cross reference.
*/
class DuplicateRefItem : public QGraphicsObject
{
	Q_OBJECT

	public:
		enum { Type = UserType + 1018 };

		/// A piece of text; a reference carries the symbol it goes to.
		struct Piece {
			QString text;
			QRectF rect;
			QPointer<Element> target;
		};

		explicit DuplicateRefItem(Element *element);

		int type() const override { return Type; }
		QRectF boundingRect() const override;
		QPainterPath shape() const override;
		void paint(QPainter *painter,
			   const QStyleOptionGraphicsItem *option,
			   QWidget *widget = nullptr) override;

		void refresh();
		QString text() const;
		QList<Piece> pieces() const { return m_pieces; }
		int referenceCount() const { return m_count; }
		Element *element() const { return m_element; }
		void paintForExport(QPainter *painter) const;

	protected:
		void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

	private:
		int pieceAt(const QPointF &pos) const;
		void place();

		Element *m_element;
		QList<Piece> m_pieces;
		QRectF m_rect;
		int m_count = 0;
		int m_hovered = -1;
		int m_pressed = -1;
};

#endif // DUPLICATEREFITEM_H
