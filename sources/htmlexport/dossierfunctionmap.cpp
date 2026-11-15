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
#include "dossierfunctionmap.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

namespace DossierFunctionMap {

namespace {

/// The value of attribute @p name in the text of a tag, XML entities decoded.
QString attribute(const QString &tag, const QString &name)
{
	const QRegularExpression re(
		QStringLiteral("(?:^|\\s)%1\\s*=\\s*\"([^\"]*)\"")
			.arg(QRegularExpression::escape(name)));
	const auto match = re.match(tag);
	if (!match.hasMatch())
		return {};
	QString value = match.captured(1);
	value.replace(QLatin1String("&lt;"), QLatin1String("<"));
	value.replace(QLatin1String("&gt;"), QLatin1String(">"));
	value.replace(QLatin1String("&quot;"), QLatin1String("\""));
	value.replace(QLatin1String("&apos;"), QLatin1String("'"));
	value.replace(QLatin1String("&amp;"), QLatin1String("&"));
	return value;
}

/// Upper case, every run of what is not a letter or a digit as one space.
QString comparable(const QString &text)
{
	QString result;
	result.reserve(text.size());
	bool space = true;
	for (const QChar c : text.toUpper()) {
		if (c.isLetterOrNumber()) {
			result += c;
			space = false;
		} else if (!space) {
			result += QLatin1Char(' ');
			space = true;
		}
	}
	return result.trimmed();
}

/// A regular expression for @p tag in comparable() text: a space may
/// be missing or added between a number and letters ("001TRF1").
QRegularExpression tagExpression(const QString &tag)
{
	QString pattern;
	QChar last;
	for (const QChar c : tag) {
		if (c == QLatin1Char(' ')) {
			pattern += QStringLiteral(" ?");
			last = QChar();
			continue;
		}
		if (!last.isNull() && last.isDigit() != c.isDigit())
			pattern += QStringLiteral(" ?");
		pattern += QRegularExpression::escape(c);
		last = c;
	}
	return QRegularExpression(QStringLiteral("(?<![0-9A-Z])%1(?![0-9A-Z])")
							  .arg(pattern));
}

struct Pattern {
	QRegularExpression expression;
	int weight;
};

/// What to look for in a text to find @p function: its tag in full, or, as a
/// weaker sign, the part of the tag after its number.
QList<Pattern> patternsOf(const Function &function)
{
	QList<Pattern> patterns;
	QSet<QString> seen;
	const QStringList alternatives = function.tag.split(QLatin1Char('/'));
	for (const QString &alternative : alternatives) {
		const QString full = comparable(alternative);
		if (full.isEmpty() || seen.contains(full))
			continue;
		seen.insert(full);
		bool only_digits = true;
		for (const QChar c : full)
			only_digits = only_digits && (c.isDigit() || c == QLatin1Char(' '));
		patterns.append({tagExpression(full), 3});

		if (only_digits)
			continue;
		QString letters = full;
		while (!letters.isEmpty() && (letters.at(0).isDigit()
									  || letters.at(0) == QLatin1Char(' ')))
			letters.remove(0, 1);
		if (letters.size() >= 3 && letters != full && !seen.contains(letters)) {
			seen.insert(letters);
			patterns.append({tagExpression(letters), 1});
		}
	}
	// A tag with words around its code: "RLX3 et éjecteur 009 EJC1"
	static const QRegularExpression code(
		QStringLiteral("(?<![0-9A-Z])\\d{3} ?[A-Z]{2,}\\d*(?![0-9A-Z])"));
	auto codes = code.globalMatch(comparable(function.tag));
	while (codes.hasNext()) {
		const QString found = codes.next().captured(0);
		if (!seen.contains(found)) {
			seen.insert(found);
			patterns.append({tagExpression(found), 3});
		}
	}
	return patterns;
}

QList<int> foliosOf(const QJsonValue &value, int folio_count)
{
	QList<int> folios;
	if (value.isObject())
		return foliosOf(value.toObject().value(QStringLiteral("folios")),
						folio_count);
	if (value.isArray()) {
		for (const QJsonValue &item : value.toArray())
			folios += foliosOf(item, folio_count);
	} else if (value.isDouble()) {
		folios << value.toInt();
	} else if (value.isString()) {
		folios = parseFolioList(value.toString(), folio_count);
	}
	QList<int> valid;
	for (int folio : folios)
		if (folio >= 1 && folio <= folio_count && !valid.contains(folio))
			valid << folio;
	std::sort(valid.begin(), valid.end());
	return valid;
}

} // namespace

QString canonicalId(const QString &id)
{
	const QString trimmed = id.trimmed();
	bool number = false;
	const int value = trimmed.toInt(&number);
	return number && value >= 0 ? QString::number(value) : trimmed;
}

QList<Function> functionsFromSvg(const QByteArray &svg)
{
	const QString text = QString::fromUtf8(svg);
	QList<Function> functions;
	QMap<QString, QList<int>> hints; // by comparable tag

	static const QRegularExpression any_tag(QStringLiteral("<[A-Za-z][^>]*>"));
	auto it = any_tag.globalMatch(text);
	while (it.hasNext()) {
		const QString tag = it.next().captured(0);
		if (tag.startsWith(QLatin1String("<a "))
			&& attribute(tag, QStringLiteral("class"))
				   .split(QLatin1Char(' ')).contains(QStringLiteral("function-link"))) {
			const QString label = attribute(tag, QStringLiteral("aria-label"));
			QStringList parts = label.split(QStringLiteral(" - "));
			if (parts.size() < 2)
				continue;
			Function function;
			function.id = parts.takeFirst().trimmed();
			function.tag = parts.takeFirst().trimmed();
			function.title = parts.join(QStringLiteral(" - ")).trimmed();
			function.anchor = attribute(tag, QStringLiteral("href"));
			if (function.anchor.startsWith(QLatin1Char('#')))
				function.anchor.remove(0, 1);
			else
				function.anchor = QStringLiteral("zoom-fonction-") + function.id;
			bool known = false;
			for (const Function &other : std::as_const(functions))
				known = known || other.id == function.id;
			if (!known)
				functions << function;
			continue;
		}
		const QString ref = attribute(tag, QStringLiteral("data-electrical-ref"));
		const QString folios = attribute(tag, QStringLiteral("data-source-folios"));
		if (!ref.isEmpty() && !folios.isEmpty())
			hints[comparable(ref)] += parseFolioList(folios, 100000);
	}

	for (Function &function : functions) {
		const QStringList alternatives = function.tag.split(QLatin1Char('/'));
		for (const QString &alternative : alternatives)
			function.hint += hints.value(comparable(alternative));
		std::sort(function.hint.begin(), function.hint.end());
		function.hint.erase(std::unique(function.hint.begin(), function.hint.end()),
							function.hint.end());
	}
	return functions;
}

QString normalizeTag(const QString &tag)
{
	QString result = tag.toUpper();
	result.remove(QLatin1Char('*'));
	result.remove(QRegularExpression(QStringLiteral("\\s+")));
	return result;
}

QList<int> parseFolioList(const QString &text, int max)
{
	QList<int> folios;
	const QStringList items = text.split(
		QRegularExpression(QStringLiteral("[\\s,;+]+")), Qt::SkipEmptyParts);
	for (const QString &item : items) {
		const QStringList bounds = item.split(QLatin1Char('-'));
		bool first_ok = false, last_ok = false;
		const int first = bounds.value(0).toInt(&first_ok);
		const int last = bounds.size() == 2 ? bounds.at(1).toInt(&last_ok) : first;
		if (!first_ok || (bounds.size() == 2 && !last_ok) || bounds.size() > 2)
			continue;
		for (int n = qMax(1, first); n <= qMin(max, last); ++n)
			folios << n;
	}
	std::sort(folios.begin(), folios.end());
	folios.erase(std::unique(folios.begin(), folios.end()), folios.end());
	return folios;
}

bool parse(const QByteArray &data, int folio_count, Map *map, QString *error)
{
	Map result;
	const QByteArray trimmed = data.trimmed();
	if (trimmed.startsWith('{')) {
		QJsonParseError json_error;
		const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &json_error);
		if (doc.isNull() || !doc.isObject()) {
			if (error)
				*error = QStringLiteral("invalid JSON: %1")
							 .arg(json_error.errorString());
			return false;
		}
		QJsonObject functions = doc.object();
		if (functions.value(QStringLiteral("functions")).isObject())
			functions = functions.value(QStringLiteral("functions")).toObject();
		for (auto it = functions.begin(); it != functions.end(); ++it) {
			if (it.key().startsWith(QLatin1Char('_')))
				continue;
			const QList<int> folios = foliosOf(it.value(), folio_count);
			if (!folios.isEmpty())
				result.insert(canonicalId(it.key()), folios);
		}
	} else {
		const QStringList lines =
			QString::fromUtf8(data).split(QRegularExpression(QStringLiteral("\\r?\\n")));
		for (QString line : lines) {
			if (line.startsWith(QChar(0xFEFF)))
				line.remove(0, 1);
			line = line.trimmed();
			if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
				continue;
			int cut = line.indexOf(QRegularExpression(QStringLiteral("[;\\t]")));
			if (cut < 0)
				cut = line.indexOf(QLatin1Char(','));
			if (cut < 0)
				continue;
			QString key = line.left(cut).trimmed();
			if (key.startsWith(QLatin1Char('"')) && key.endsWith(QLatin1Char('"')))
				key = key.mid(1, key.size() - 2);
			QString value = line.mid(cut + 1).trimmed();
			if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
				value = value.mid(1, value.size() - 2);
			const QList<int> folios = parseFolioList(value, folio_count);
			if (key.isEmpty() || folios.isEmpty())
				continue; // header line, or a function with no folio
			result.insert(canonicalId(key), folios);
		}
	}
	if (map)
		*map = result;
	return true;
}

QByteArray toJson(const Map &map, const QList<Function> &functions)
{
	QJsonObject entries;
	for (const Function &function : functions) {
		QJsonArray folios;
		for (int folio : map.value(canonicalId(function.id)))
			folios.append(folio);
		QJsonObject entry;
		entry.insert(QStringLiteral("tag"), function.tag);
		entry.insert(QStringLiteral("title"), function.title);
		entry.insert(QStringLiteral("folios"), folios);
		entries.insert(function.id, entry);
	}
	QJsonObject root;
	root.insert(QStringLiteral("_comment"),
				QStringLiteral("Folios (from 1) of each function of the overview "
							   "picture. Edit the lists and pass this file back "
							   "with --html-map."));
	root.insert(QStringLiteral("functions"), entries);
	return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

Map deduce(const QList<Function> &functions,
		   const QList<QStringList> &folio_texts, int max_folios)
{
	QList<QStringList> texts;
	texts.reserve(folio_texts.size());
	for (const QStringList &folio : folio_texts) {
		QStringList normalized;
		normalized.reserve(folio.size());
		for (const QString &text : folio)
			normalized << comparable(text);
		texts << normalized;
	}

	Map map;
	QList<QList<QPair<int, int>>> all_scores; // by function: score, folio
	QList<int> matches(texts.size() + 1, 0);  // functions seen in each folio
	for (const Function &function : functions) {
		const QList<Pattern> patterns = patternsOf(function);
		QList<QPair<int, int>> scores;
		for (int folio = 1; folio <= texts.size(); ++folio) {
			int score = 0;
			for (const QString &text : texts.at(folio - 1))
				for (const Pattern &pattern : patterns)
					if (text.size() >= 2 && pattern.expression.match(text).hasMatch())
						score += pattern.weight;
			if (score > 0)
				++matches[folio];
			if (function.hint.contains(folio))
				score += 2;
			if (score > 0)
				scores.append({score, folio});
		}
		all_scores << scores;
	}

	// A folio which names a third of the functions (an index, a legend, a
	// general layout) tells nothing about one of them: it is only kept when
	// the picture itself points to it, or when nothing else is left.
	const bool filter = functions.size() >= 6;
	const auto general = [&](int folio) {
		return filter && matches.at(folio) * 3 > functions.size();
	};
	for (int i = 0; i < functions.size(); ++i) {
		const Function &function = functions.at(i);
		QList<QPair<int, int>> scores;
		for (const auto &score : std::as_const(all_scores.at(i)))
			if (!general(score.second) || function.hint.contains(score.second))
				scores.append(score);
		if (scores.isEmpty())
			scores = all_scores.at(i);
		if (scores.isEmpty())
			continue;
		std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
			return a.first != b.first ? a.first > b.first : a.second < b.second;
		});
		const int best = scores.first().first;
		QList<int> folios;
		for (const auto &score : std::as_const(scores)) {
			if (folios.size() >= max_folios || score.first * 4 < best)
				break;
			folios << score.second;
		}
		std::sort(folios.begin(), folios.end());
		map.insert(canonicalId(function.id), folios);
	}
	return map;
}

} // namespace DossierFunctionMap
