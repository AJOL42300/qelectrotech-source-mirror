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
#include "islandnumbering.h"

#include <algorithm>

namespace IslandNumbering
{

/**
	@brief childLabel
	@param parent : label of the island (the coupler)
	@param number : number of the card
	@return the label of the card, e.g. "A1.10"
*/
QString childLabel(const QString &parent, int number)
{
	return parent + QLatin1Char('.') + QString::number(number);
}

/**
	@brief headLabel
	@param parent : identifier of the island
	@return the label of its head, e.g. "A1.00"
*/
QString headLabel(const QString &parent)
{
	return parent + QStringLiteral(".00");
}

/**
	@brief islandOf
	@param label : label of an element
	@return the identifier of the island @p label belongs to: "A1" for
	"A1.20", and @p label itself when it has no ".number" at its end
*/
QString islandOf(const QString &label)
{
	const int dot = label.lastIndexOf(QLatin1Char('.'));
	int number = 0;
	if (dot > 0 && numberOf(label, label.left(dot), &number)) {
		return label.left(dot);
	}
	return label;
}

/**
	@brief appearancePaths
	A card can be drawn in three ways, kept in three elements of the same
	folder: the symbol "name.elmt", the realistic view "name_realistic.elmt"
	and the table of its channels "name_table.elmt".
	@param path : path of one of them, in a collection
	@return the paths of the two others, in the order symbol, realistic view,
	table; they may not exist
*/
QStringList appearancePaths(const QString &path)
{
	const QString extension = QStringLiteral(".elmt");
	if (!path.endsWith(extension)) {
		return QStringList();
	}
	QString base = path.left(path.length() - extension.length());
	for (const QString &suffix : {QStringLiteral("_realistic"),
								  QStringLiteral("_table")}) {
		if (base.endsWith(suffix)) {
			base.chop(suffix.length());
			break;
		}
	}
	QStringList all;
	all << base + extension
		<< base + QStringLiteral("_realistic") + extension
		<< base + QStringLiteral("_table") + extension;
	all.removeAll(path);
	return all;
}

/**
	@brief numberOf
	@param label : label of an element
	@param parent : label of the island
	@param number : set to the number of the card when @p label is a direct
	card of @p parent (digits only after "parent."); can be nullptr
	@return true if @p label is a direct card of @p parent
*/
bool numberOf(const QString &label, const QString &parent, int *number)
{
	const QString prefix = parent + QLatin1Char('.');
	if (parent.isEmpty() || !label.startsWith(prefix)) {
		return false;
	}
	const QString suffix = label.mid(prefix.length());
	if (suffix.isEmpty() || suffix.length() > 6) {
		return false;
	}
	for (const QChar c : suffix) {
		if (c < QLatin1Char('0') || c > QLatin1Char('9')) {
			return false;
		}
	}
	if (number) {
		*number = suffix.toInt();
	}
	return true;
}

/**
	@brief numbersAfter
	Free numbers to give to @p count new cards.
	At the end of the island (@p after = -1) they continue by steps of 10
	from the last number. Elsewhere they are spread evenly in the gap
	between @p after and the next card, so one card inserted between .10 and
	.20 gets .15, and no other card is renumbered.
	@param used : numbers already taken, in any order
	@param count : how many numbers are needed
	@param after : -1 for the end, 0 for before the first card, otherwise
	the number of the card the new ones follow
	@return the numbers in increasing order, or an empty list if the gap is
	too small for @p count numbers
*/
QList<int> numbersAfter(const QList<int> &used, int count, int after)
{
	QList<int> result;
	if (count <= 0) {
		return result;
	}
	QList<int> sorted = used;
	std::sort(sorted.begin(), sorted.end());

	int low = 0;
	int high = -1;
	if (after < 0) {
		low = sorted.isEmpty() ? 0 : sorted.last();
	} else {
		low = after;
		for (int n : sorted) {
			if (n > low) {
				high = n;
				break;
			}
		}
	}

	if (high < 0) {
		int next = (low / step + 1) * step;
		for (int i = 0; i < count; ++i) {
			result << next;
			next += step;
		}
		return result;
	}

	const int gap = high - low;
	if (gap - 1 < count) {
		return result;
	}
	for (int i = 0; i < count; ++i) {
		result << low + gap * (i + 1) / (count + 1);
	}
	return result;
}

/**
	@brief isTaken
	@param parent : label of an island
	@param labels : the labels of all the elements of the project
	@return true if an element is labelled @p parent, or is one of its cards
*/
bool isTaken(const QString &parent, const QStringList &labels)
{
	for (const QString &label : labels) {
		if (label == parent || numberOf(label, parent, nullptr)) {
			return true;
		}
	}
	return false;
}

/**
	@brief nextParent
	The label for a copy of an island: the number which ends @p parent goes
	up by one ("A1" gives "A2", the zeros are kept) until no element
	of @p labels carries it. A label without a number gets one: "CC" gives
	"CC 2".
	@param parent : label of the island to copy
	@param labels : the labels of all the elements of the project
	@return a free label for the copy
*/
QString nextParent(const QString &parent, const QStringList &labels)
{
	int digits = 0;
	while (digits < parent.length()
		   && parent.at(parent.length() - 1 - digits).isDigit()) {
		++digits;
	}
	QString head = parent.left(parent.length() - digits);
	int width = digits;
	qlonglong value = digits ? parent.right(digits).toLongLong() : 1;
	if (digits == 0 && !head.isEmpty()
		&& !head.endsWith(QLatin1Char(' '))) {
		head += QLatin1Char(' ');
	}
	QString candidate;
	do {
		++value;
		candidate = head
			+ QString::number(value).rightJustified(width, QLatin1Char('0'));
	} while (isTaken(candidate, labels));
	return candidate;
}

/**
	@brief copyLabels
	@param parent : label of the island
	@param count : number of cards
	@return the labels of @p count cards, by steps of 10 in their order:
	"A2.10", "A2.20"...
*/
QStringList copyLabels(const QString &parent, int count)
{
	QStringList labels;
	for (int i = 1; i <= count; ++i) {
		labels << childLabel(parent, i * step);
	}
	return labels;
}

/**
	@brief childFilter
	@param column : column holding the label
	@param parent : label of the island
	@param with_parent : true to also accept the island itself
	@return the SQL condition on @p column: the label of a direct card of
	the island, so "P.10" but neither "P.10.5", "P.x" nor "P1.10"
*/
static QString childFilter(const QString &column, const QString &parent,
						   bool with_parent)
{
		//LIKE metacharacters of the label are escaped with a backslash
	QString like = parent;
	like.replace(QLatin1String("\\"), QLatin1String("\\\\"))
		.replace(QLatin1String("%"), QLatin1String("\\%"))
		.replace(QLatin1String("_"), QLatin1String("\\_"))
		.replace(QLatin1Char('\''), QLatin1String("''"));
	QString literal = parent;
	literal.replace(QLatin1Char('\''), QLatin1String("''"));

	const QString child = QStringLiteral(
		"(%1 LIKE '%2.%' ESCAPE '\\' "
		"AND %1 NOT LIKE '%2.%.%' ESCAPE '\\' "
		"AND LENGTH(%1) > LENGTH('%3') + 1 "
		"AND SUBSTR(%1, LENGTH('%3') + 2) NOT GLOB '*[^0-9]*')")
			.arg(column, like, literal);
	if (!with_parent) {
		return child;
	}
	return QStringLiteral("(%1 = '%2' OR %3)").arg(column, literal, child);
}

static QString numberOrder(const QString &column, const QString &parent)
{
	QString literal = parent;
	literal.replace(QLatin1Char('\''), QLatin1String("''"));
	return QStringLiteral(
		"CASE WHEN %1 = '%2' THEN 0 "
		"ELSE CAST(SUBSTR(%1, LENGTH('%2') + 2) AS INTEGER) END")
			.arg(column, literal);
}

/**
	@brief cardsQuery
	@param parent : label of the island
	@return the SQL, to run on element_nomenclature_view, listing the direct
	cards of the island ordered by number
*/
QString cardsQuery(const QString &parent)
{
	return QStringLiteral(
		"SELECT label, designation, manufacturer, manufacturer_reference "
		"FROM element_nomenclature_view WHERE %1 ORDER BY %2")
			.arg(childFilter(QStringLiteral("label"), parent, false),
				 numberOrder(QStringLiteral("label"), parent));
}

/**
	@brief ioQuery
	@param parent : label of the island
	@return the SQL, to run on the project database, listing the inputs
	and outputs (the I/O table of each PLC element, see qet_plc_io) of the
	island and of its cards: one row per channel, with the label and the
	folio of the card it belongs to, ordered by card number then channel
*/
QString ioQuery(const QString &parent)
{
	return QStringLiteral(
		"SELECT plc AS card, entry AS channel, COALESCE(address, '') AS address, "
		"COALESCE(type, '') AS type, COALESCE(function, '') AS function, "
		"COALESCE(comment, '') AS comment, "
		"COALESCE(plc_folio_number, '') AS folio "
		"FROM qet_plc_io WHERE %1 ORDER BY %2, entry")
			.arg(childFilter(QStringLiteral("plc"), parent, true),
				 numberOrder(QStringLiteral("plc"), parent));
}

}
