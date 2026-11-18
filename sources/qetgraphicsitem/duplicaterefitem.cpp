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
#include "duplicaterefitem.h"

#include "../componentreferences.h"
#include "../diagram.h"
#include "../qetapp.h"
#include "../qetproject.h"
#include "../shownkinds.h"
#include "element.h"
#include "qetgraphicsitem.h"

#include <QCoreApplication>
#include <QFontMetricsF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace {
	QString appearanceSuffix(ComponentReferences::Appearance appearance,
				 bool named)
	{
		if (!named) return QString();
		return QStringLiteral(" (%1)")
			.arg(ComponentReferences::appearanceName(appearance));
	}
}

DuplicateRefItem::DuplicateRefItem(Element *element) :
	QGraphicsObject(element),
	m_element(element)
{
	ShownKinds::tag(this, ShownKinds::CrossReferences);
	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
	setFlag(QGraphicsItem::ItemStacksBehindParent, false);
	ShownKinds::setVisible(this, false);
}

QRectF DuplicateRefItem::boundingRect() const
{
	return m_rect.adjusted(-1, -1, 1, 1);
}

/// Only the references take mouse events: the rest of the text is not a
/// target, a click there goes to what is behind.
QPainterPath DuplicateRefItem::shape() const
{
	QPainterPath path;
	for (const Piece &piece : m_pieces) {
		if (piece.target) path.addRect(piece.rect);
	}
	return path;
}

/**
	Work out the text again from the index: what the symbol refers to, and
	where the text goes.
*/
void DuplicateRefItem::refresh()
{
	using namespace ComponentReferences;

	QList<Reference> references;
	Diagram *diagram = m_element->diagram();
	QETProject *project = diagram ? diagram->project() : nullptr;
	if (project && ComponentReferences::isEnabled(project)
			&& !ComponentReferenceIndex::isHidden(m_element)) {
		if (ComponentReferenceIndex *index =
				ComponentReferenceIndex::of(project)) {
			references = index->referencesOf(m_element);
		}
	}

	prepareGeometryChange();
	m_pieces.clear();
	m_rect = QRectF();
	m_count = references.size();
	m_hovered = -1;
	if (references.isEmpty() || !project) {
		ShownKinds::setVisible(this, false);
		update();
		return;
	}

	const Settings settings = project->componentReferences();
	const Appearance own = appearanceOf(m_element->location().path());
	bool named = own != Appearance::Schematic;
	for (const Reference &reference : references) {
		if (reference.appearance != Appearance::Schematic) named = true;
	}

	const QFont font = QETApp::diagramTextsFont(5);
	const QFontMetricsF metrics(font);
	const qreal height = metrics.height();
	const QString arrow = QStringLiteral("\u2192 ");
	const QString separator = QStringLiteral(", ");

	auto add = [&](qreal x, qreal y, const QString &text, Element *target) {
		Piece piece;
		piece.text = text;
		piece.rect = QRectF(x, y, metrics.horizontalAdvance(text), height);
		piece.target = target;
		m_pieces << piece;
		return piece.rect.right();
	};

	if (settings.style == Style::Compact) {
		qreal x = add(0, 0, arrow, nullptr);
		for (int i = 0; i < references.size(); ++i) {
			const Reference &reference = references.at(i);
			if (i > 0) x = add(x, 0, separator, nullptr);
			x = add(x, 0,
				QCoreApplication::translate("DuplicateRefItem",
							    "folio %1")
					.arg(reference.folio)
					+ appearanceSuffix(reference.appearance, named),
				reference.element);
		}
	} else {
		for (int i = 0; i < references.size(); ++i) {
			const Reference &reference = references.at(i);
			const qreal y = i * height;
			const qreal x = add(0, y, arrow, nullptr);
			add(x, y,
			    QCoreApplication::translate("DuplicateRefItem",
							"folio %1, %2")
				.arg(reference.folio)
				.arg(reference.position)
				+ appearanceSuffix(reference.appearance, named),
			    reference.element);
		}
	}
	for (const Piece &piece : m_pieces) {
		m_rect = m_rect.united(piece.rect);
	}
	place();
	ShownKinds::setVisible(this, true);
	update();
}

QString DuplicateRefItem::text() const
{
	QStringList lines;
	QString line;
	qreal y = 0;
	for (const Piece &piece : m_pieces) {
		if (!line.isEmpty() && piece.rect.top() > y) {
			lines << line;
			line.clear();
		}
		y = piece.rect.top();
		line += piece.text;
	}
	if (!line.isEmpty()) lines << line;
	return lines.join(QLatin1Char('\n'));
}

/// Under the symbol or on its right, as the project says
void DuplicateRefItem::place()
{
	const ComponentReferences::Settings settings =
		m_element->diagram()->project()->componentReferences();
	QRectF symbol = m_element->boundingRect();
	// the texts of the symbol (its label...) are not part of its rectangle
	for (QGraphicsItem *child : m_element->childItems()) {
		if (child == this || !child->isVisible()
				|| child->type() == DuplicateRefItem::Type) continue;
		symbol |= child->mapToParent(child->boundingRect()).boundingRect();
	}
	if (settings.placement == ComponentReferences::Placement::Right) {
		setPos(symbol.right() + 3, symbol.top());
	} else {
		setPos(symbol.left(), symbol.bottom() + 2);
	}
}

void DuplicateRefItem::paint(QPainter *painter,
			     const QStyleOptionGraphicsItem *option,
			     QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)
	paintForExport(painter);
}

void DuplicateRefItem::paintForExport(QPainter *painter) const
{
	painter->save();
	QFont font = QETApp::diagramTextsFont(5);
	for (int i = 0; i < m_pieces.size(); ++i) {
		const Piece &piece = m_pieces.at(i);
		const bool link = piece.target;
		font.setUnderline(link && i == m_hovered);
		painter->setFont(font);
		painter->setPen(link ? QColor(0, 60, 160) : QColor(Qt::black));
		painter->drawText(piece.rect,
				  Qt::AlignLeft | Qt::AlignVCenter,
				  piece.text);
	}
	painter->restore();
}

int DuplicateRefItem::pieceAt(const QPointF &pos) const
{
	for (int i = 0; i < m_pieces.size(); ++i) {
		if (m_pieces.at(i).target && m_pieces.at(i).rect.contains(pos))
			return i;
	}
	return -1;
}

void DuplicateRefItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	const int hovered = pieceAt(event->pos());
	if (hovered != m_hovered) {
		m_hovered = hovered;
		setCursor(hovered >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
		update();
	}
	QGraphicsObject::hoverMoveEvent(event);
}

void DuplicateRefItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered = -1;
	unsetCursor();
	update();
	QGraphicsObject::hoverLeaveEvent(event);
}

void DuplicateRefItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	m_pressed = pieceAt(event->pos());
	if (m_pressed < 0) {
		event->ignore();
		return;
	}
	event->accept();
}

void DuplicateRefItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	const int pressed = m_pressed;
	m_pressed = -1;
	if (pressed < 0 || pressed != pieceAt(event->pos())) {
		QGraphicsObject::mouseReleaseEvent(event);
		return;
	}
	Element *target = m_pieces.at(pressed).target;
	event->accept();
	if (target) QetGraphicsItem::showItem(target);
}
