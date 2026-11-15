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
#include "svgtoelmt.h"

#include "pictureurl.h"
#include "svgtoelmtpalette.h"

#include <QColor>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QMimeData>
#include <QPainterPath>
#include <QPolygonF>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>
#include <QTransform>
#include <QUrl>
#include <QUuid>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

constexpr int grid_step = 10;
constexpr int max_depth = 32;

/// One subpath, in the coordinates of the SVG.
struct Sub {
	QPainterPath path;
	bool closed = false;
};

struct Style {
	QString fill = QStringLiteral("black");
	QString stroke = QStringLiteral("none");
	double strokeWidth = 1;
	double fillOpacity = 1;
	double strokeOpacity = 1;
	bool hidden = false;
};

struct Shape {
	enum class Kind { Polygons, Rectangle, Ellipse };
	Kind kind = Kind::Polygons;
	QRectF rect;           // Rectangle, Ellipse
	double rx = 0, ry = 0; // Rectangle
	QList<Sub> subs;       // Polygons
	Style style;
	double scale = 1;      // of the transforms on the way to the root
	QRectF bounds;
};

struct CssRule {
	QString selector;
	QList<QPair<QString, QString>> declarations;
};

/// QRectF::united() ignores an empty rectangle, such as a straight line.
QRectF unite(const QRectF &a, const QRectF &b)
{
	const double left = std::min(a.left(), b.left());
	const double top = std::min(a.top(), b.top());
	return QRectF(QPointF(left, top),
				  QPointF(std::max(a.right(), b.right()),
						  std::max(a.bottom(), b.bottom())));
}

QString localName(const QDomElement &e)
{
	const QString name = e.tagName();
	return name.mid(name.lastIndexOf(QLatin1Char(':')) + 1);
}

/// Reads the numbers and flags of a path or a list of points.
class Scanner
{
	public:
		explicit Scanner(const QString &text) : m_text(text) {}

		void skipSeparators(bool comma = true)
		{
			while (m_pos < m_text.size()
				   && (m_text.at(m_pos).isSpace()
					   || (comma && m_text.at(m_pos) == QLatin1Char(','))))
				++m_pos;
		}
		bool atEnd() { skipSeparators(); return m_pos >= m_text.size(); }
		QChar peek() { skipSeparators(); return m_pos < m_text.size() ? m_text.at(m_pos) : QChar(); }
		void next() { ++m_pos; }

		bool number(double *value)
		{
			skipSeparators();
			const int start = m_pos;
			int i = m_pos;
			if (i < m_text.size() && (m_text.at(i) == QLatin1Char('+') || m_text.at(i) == QLatin1Char('-')))
				++i;
			int digits = 0;
			while (i < m_text.size() && m_text.at(i).isDigit()) { ++i; ++digits; }
			if (i < m_text.size() && m_text.at(i) == QLatin1Char('.')) {
				++i;
				while (i < m_text.size() && m_text.at(i).isDigit()) { ++i; ++digits; }
			}
			if (!digits) return false;
			if (i < m_text.size() && (m_text.at(i) == QLatin1Char('e') || m_text.at(i) == QLatin1Char('E'))) {
				int j = i + 1;
				if (j < m_text.size() && (m_text.at(j) == QLatin1Char('+') || m_text.at(j) == QLatin1Char('-')))
					++j;
				if (j < m_text.size() && m_text.at(j).isDigit()) {
					while (j < m_text.size() && m_text.at(j).isDigit()) ++j;
					i = j;
				}
			}
			bool ok = false;
			*value = m_text.mid(start, i - start).toDouble(&ok);
			m_pos = i;
			return ok && std::isfinite(*value);
		}

		/// An arc flag: one digit, which needs no separator after it.
		bool flag(bool *value)
		{
			skipSeparators();
			if (m_pos >= m_text.size()) return false;
			const QChar c = m_text.at(m_pos);
			if (c != QLatin1Char('0') && c != QLatin1Char('1')) return false;
			*value = c == QLatin1Char('1');
			++m_pos;
			return true;
		}

	private:
		const QString &m_text;
		int m_pos = 0;
};

double lengthValue(const QString &text, double fallback = 0)
{
	static const QRegularExpression rx(QStringLiteral("^\\s*([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)"));
	const auto m = rx.match(text);
	return m.hasMatch() ? m.captured(1).toDouble() : fallback;
}

QList<double> numberList(const QString &text)
{
	QList<double> list;
	Scanner s(text);
	double v = 0;
	while (s.number(&v)) list << v;
	return list;
}

/// The transform attribute: matrix, translate, scale, rotate, skewX, skewY.
QTransform parseTransform(const QString &text)
{
	QTransform result;
	static const QRegularExpression rx(QStringLiteral("(\\w+)\\s*\\(([^)]*)\\)"));
	auto it = rx.globalMatch(text);
	while (it.hasNext()) {
		const auto m = it.next();
		const QString kind = m.captured(1);
		const QList<double> a = numberList(m.captured(2));
		QTransform t;
		if (kind == QLatin1String("matrix") && a.size() == 6)
			t = QTransform(a[0], a[1], a[2], a[3], a[4], a[5]);
		else if (kind == QLatin1String("translate") && !a.isEmpty())
			t = QTransform::fromTranslate(a[0], a.value(1, 0));
		else if (kind == QLatin1String("scale") && !a.isEmpty())
			t = QTransform::fromScale(a[0], a.value(1, a[0]));
		else if (kind == QLatin1String("rotate") && !a.isEmpty()) {
			t.translate(a.value(1, 0), a.value(2, 0));
			t.rotate(a[0]);
			t.translate(-a.value(1, 0), -a.value(2, 0));
		}
		else if (kind == QLatin1String("skewX") && !a.isEmpty())
			t.shear(std::tan(qDegreesToRadians(a[0])), 0);
		else if (kind == QLatin1String("skewY") && !a.isEmpty())
			t.shear(0, std::tan(qDegreesToRadians(a[0])));
		result = t * result;
	}
	return result;
}

/// Mean scale of a transform, to scale the width of a stroke.
double transformScale(const QTransform &t)
{
	return std::sqrt(std::abs(t.determinant()));
}

/// An arc of a path, as the cubic curves that draw it.
void arcTo(QPainterPath &path, QPointF from, double rx, double ry,
		   double degrees, bool large, bool sweep, QPointF to)
{
	rx = std::abs(rx);
	ry = std::abs(ry);
	if (from == to) return;
	if (rx < 1e-9 || ry < 1e-9) { path.lineTo(to); return; }

	const double phi = qDegreesToRadians(degrees);
	const double c = std::cos(phi), s = std::sin(phi);
	const double dx = (from.x() - to.x()) / 2, dy = (from.y() - to.y()) / 2;
	const double x1 = c * dx + s * dy, y1 = -s * dx + c * dy;
	const double lambda = x1 * x1 / (rx * rx) + y1 * y1 / (ry * ry);
	if (lambda > 1) { rx *= std::sqrt(lambda); ry *= std::sqrt(lambda); }

	const double num = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
	const double den = rx * rx * y1 * y1 + ry * ry * x1 * x1;
	const double coef = (large == sweep ? -1 : 1) * std::sqrt(std::max(0.0, num / den));
	const double cx1 = coef * rx * y1 / ry, cy1 = -coef * ry * x1 / rx;
	const double cx = c * cx1 - s * cy1 + (from.x() + to.x()) / 2;
	const double cy = s * cx1 + c * cy1 + (from.y() + to.y()) / 2;

	const double start = std::atan2((y1 - cy1) / ry, (x1 - cx1) / rx);
	double delta = std::atan2((-y1 - cy1) / ry, (-x1 - cx1) / rx) - start;
	if (!sweep && delta > 0) delta -= 2 * M_PI;
	if (sweep && delta < 0) delta += 2 * M_PI;

	const int segments = qMax(1, int(std::ceil(std::abs(delta) / (M_PI / 2) - 1e-9)));
	const double step = delta / segments;
	const double k = 4.0 / 3.0 * std::tan(step / 4);
	auto map = [&](double x, double y) {
		return QPointF(cx + rx * x * c - ry * y * s, cy + rx * x * s + ry * y * c);
	};
	for (int i = 0; i < segments; ++i) {
		const double a1 = start + i * step, a2 = a1 + step;
		const QPointF c1 = map(std::cos(a1) - k * std::sin(a1), std::sin(a1) + k * std::cos(a1));
		const QPointF c2 = map(std::cos(a2) + k * std::sin(a2), std::sin(a2) - k * std::cos(a2));
		path.cubicTo(c1, c2, i == segments - 1 ? to : map(std::cos(a2), std::sin(a2)));
	}
}

/// The d attribute of a path, as subpaths. What follows an error is dropped.
QList<Sub> parsePath(const QString &d)
{
	QList<Sub> subs;
	Scanner s(d);
	QPointF cur, start, control;
	QChar cmd, previous;

	auto begin = [&](QPointF p) {
		Sub sub;
		sub.path.moveTo(p);
		subs << sub;
		cur = start = p;
	};
	auto draw = [&]() -> QPainterPath & {
		if (subs.isEmpty() || subs.last().closed) begin(cur);
		return subs.last().path;
	};

	while (!s.atEnd()) {
		const QChar c = s.peek();
		if (c.isLetter()) {
			cmd = c;
			s.next();
		} else if (cmd.isNull()) {
			break;
		}
		const bool rel = cmd.isLower();
		const QChar up = cmd.toUpper();
		const QPointF origin = rel ? cur : QPointF();
		double a = 0, b = 0, e = 0, f = 0, g = 0, h = 0;
		bool ok = true;

		if (up == QLatin1Char('Z')) {
			if (!subs.isEmpty() && !subs.last().closed) {
				subs.last().path.closeSubpath();
				subs.last().closed = true;
			}
			cur = start;
			previous = up;
			cmd = QChar();
			continue;
		}
		if (up == QLatin1Char('M')) {
			if (!(ok = s.number(&a) && s.number(&b))) break;
			begin(origin + QPointF(a, b));
			cmd = rel ? QLatin1Char('l') : QLatin1Char('L');
		} else if (up == QLatin1Char('L')) {
			if (!(ok = s.number(&a) && s.number(&b))) break;
			const QPointF p = origin + QPointF(a, b);
			draw().lineTo(p);
			cur = p;
		} else if (up == QLatin1Char('H')) {
			if (!(ok = s.number(&a))) break;
			const QPointF p(rel ? cur.x() + a : a, cur.y());
			draw().lineTo(p);
			cur = p;
		} else if (up == QLatin1Char('V')) {
			if (!(ok = s.number(&a))) break;
			const QPointF p(cur.x(), rel ? cur.y() + a : a);
			draw().lineTo(p);
			cur = p;
		} else if (up == QLatin1Char('C') || up == QLatin1Char('S')) {
			QPointF c1;
			if (up == QLatin1Char('C')) {
				if (!(ok = s.number(&a) && s.number(&b))) break;
				c1 = origin + QPointF(a, b);
			} else {
				const bool smooth = previous == QLatin1Char('C') || previous == QLatin1Char('S');
				c1 = smooth ? 2 * cur - control : cur;
			}
			if (!(ok = s.number(&e) && s.number(&f) && s.number(&g) && s.number(&h))) break;
			const QPointF c2 = origin + QPointF(e, f), p = origin + QPointF(g, h);
			draw().cubicTo(c1, c2, p);
			control = c2;
			cur = p;
		} else if (up == QLatin1Char('Q') || up == QLatin1Char('T')) {
			QPointF c1;
			if (up == QLatin1Char('Q')) {
				if (!(ok = s.number(&a) && s.number(&b))) break;
				c1 = origin + QPointF(a, b);
			} else {
				const bool smooth = previous == QLatin1Char('Q') || previous == QLatin1Char('T');
				c1 = smooth ? 2 * cur - control : cur;
			}
			if (!(ok = s.number(&e) && s.number(&f))) break;
			const QPointF p = origin + QPointF(e, f);
			draw().quadTo(c1, p);
			control = c1;
			cur = p;
		} else if (up == QLatin1Char('A')) {
			bool large = false, sweep = false;
			if (!(ok = s.number(&a) && s.number(&b) && s.number(&e) && s.flag(&large)
					  && s.flag(&sweep) && s.number(&g) && s.number(&h))) break;
			const QPointF p = origin + QPointF(g, h);
			QPainterPath &path = draw();
			arcTo(path, cur, a, b, e, large, sweep, p);
			cur = p;
		} else {
			break;
		}
		previous = up;
	}
	return subs;
}

/// Parse "name: value; ..." into pairs.
QList<QPair<QString, QString>> parseDeclarations(const QString &text)
{
	QList<QPair<QString, QString>> list;
	const QStringList parts = text.split(QLatin1Char(';'), Qt::SkipEmptyParts);
	for (const QString &part : parts) {
		const int colon = part.indexOf(QLatin1Char(':'));
		if (colon <= 0) continue;
		list << qMakePair(part.left(colon).trimmed().toLower(), part.mid(colon + 1).trimmed());
	}
	return list;
}

QList<CssRule> parseCss(const QString &text)
{
	QList<CssRule> rules;
	QString css = text;
	css.remove(QRegularExpression(QStringLiteral("/\\*.*?\\*/"),
								  QRegularExpression::DotMatchesEverythingOption));
	static const QRegularExpression rx(QStringLiteral("([^{}@]+)\\{([^}]*)\\}"));
	auto it = rx.globalMatch(css);
	while (it.hasNext()) {
		const auto m = it.next();
		const auto declarations = parseDeclarations(m.captured(2));
		const QStringList selectors = m.captured(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
		for (const QString &selector : selectors)
			rules << CssRule{selector.trimmed(), declarations};
	}
	return rules;
}

QColor blendWithWhite(const QColor &color, double opacity)
{
	opacity = qBound(0.0, opacity, 1.0);
	auto mix = [&](int v) { return qRound(255 * (1 - opacity) + v * opacity); };
	return QColor(mix(color.red()), mix(color.green()), mix(color.blue()));
}

bool parseColor(const QString &text, QColor *color)
{
	const QString v = text.trimmed().toLower();
	if (v.isEmpty() || v == QLatin1String("none") || v == QLatin1String("transparent"))
		return false;
	if (v == QLatin1String("currentcolor")) { *color = Qt::black; return true; }
	if (v.startsWith(QLatin1String("rgb"))) {
		static const QRegularExpression rx(QStringLiteral("rgba?\\(([^)]*)\\)"));
		const auto m = rx.match(v);
		if (!m.hasMatch()) return false;
		const QStringList parts = m.captured(1).split(QRegularExpression(QStringLiteral("[,\\s/]+")),
													  Qt::SkipEmptyParts);
		if (parts.size() < 3) return false;
		int rgb[3];
		for (int i = 0; i < 3; ++i) {
			const double n = lengthValue(parts.at(i));
			rgb[i] = qBound(0, qRound(parts.at(i).contains(QLatin1Char('%')) ? n * 2.55 : n), 255);
		}
		*color = QColor(rgb[0], rgb[1], rgb[2]);
		return true;
	}
	const QColor c(v);
	if (!c.isValid()) return false;
	*color = c;
	return true;
}

QString nearestName(const QColor &color)
{
	const char *best = "black";
	long best_distance = -1;
	for (const auto &entry : SvgToElmtPalette::colors) {
		const long dr = color.red() - entry.red;
		const long dg = color.green() - entry.green;
		const long db = color.blue() - entry.blue;
		const long distance = 2 * dr * dr + 4 * dg * dg + 3 * db * db;
		if (best_distance < 0 || distance < best_distance) {
			best_distance = distance;
			best = entry.name;
		}
	}
	return QString::fromLatin1(best);
}

QString number(double v)
{
	const double rounded = qRound(v * 100.0) / 100.0;
	return QString::number(rounded == 0 ? 0.0 : rounded, 'g', 8);
}

class Converter
{
	public:
		Converter(const QDomElement &root, SvgToElmt::Result *result)
			: m_root(root), m_result(result) {}

		void run()
		{
			collect(m_root);
			walk(m_root, QTransform(), Style(), 0);
			if (m_ignored_images)
				warn(QStringLiteral("%1 picture(s) (<image>) not converted").arg(m_ignored_images));
			if (m_ignored_texts)
				warn(QStringLiteral("%1 text(s) not converted").arg(m_ignored_texts));
		}

		QList<Shape> shapes;
		QList<SvgToElmt::Terminal> markers;

		/// The colour of a fill or a stroke, blended with white by its
		/// opacity. False: nothing is drawn.
		bool paintOf(const QString &text, double opacity, QColor *color) const
		{
			return paint(text, opacity, color);
		}

	private:
		void warn(const QString &text) { m_result->warnings << text; }

		void collect(const QDomElement &e)
		{
			const QString id = e.attribute(QStringLiteral("id"));
			if (!id.isEmpty() && !m_ids.contains(id)) m_ids.insert(id, e);
			if (localName(e) == QLatin1String("style"))
				m_rules << parseCss(e.text());
			for (QDomElement c = e.firstChildElement(); !c.isNull(); c = c.nextSiblingElement())
				collect(c);
		}

		void apply(Style &style, const QString &name, const QString &value) const
		{
			if (name == QLatin1String("fill")) style.fill = value;
			else if (name == QLatin1String("stroke")) style.stroke = value;
			else if (name == QLatin1String("stroke-width")) style.strokeWidth = lengthValue(value, 1);
			else if (name == QLatin1String("fill-opacity")) style.fillOpacity *= lengthValue(value, 1);
			else if (name == QLatin1String("stroke-opacity")) style.strokeOpacity *= lengthValue(value, 1);
			else if (name == QLatin1String("opacity")) {
				style.fillOpacity *= lengthValue(value, 1);
				style.strokeOpacity *= lengthValue(value, 1);
			}
			else if (name == QLatin1String("display") && value == QLatin1String("none")) style.hidden = true;
			else if (name == QLatin1String("visibility") && value != QLatin1String("visible")
					 && value != QLatin1String("inherit")) style.hidden = true;
		}

		Style styleOf(const QDomElement &e, Style style) const
		{
			static const QStringList presentation = {
				QStringLiteral("fill"), QStringLiteral("stroke"), QStringLiteral("stroke-width"),
				QStringLiteral("fill-opacity"), QStringLiteral("stroke-opacity"),
				QStringLiteral("opacity"), QStringLiteral("display"), QStringLiteral("visibility")};
			for (const QString &name : presentation)
				if (e.hasAttribute(name)) apply(style, name, e.attribute(name).trimmed());

			const QString tag = localName(e);
			const QStringList classes = e.attribute(QStringLiteral("class"))
										 .split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
			for (const CssRule &rule : m_rules) {
				bool match = rule.selector == QLatin1String("*") || rule.selector == tag;
				for (const QString &cls : classes)
					match = match || rule.selector == QLatin1Char('.') + cls
							|| rule.selector == tag + QLatin1Char('.') + cls;
				if (!match) continue;
				for (const auto &d : rule.declarations) apply(style, d.first, d.second);
			}
			for (const auto &d : parseDeclarations(e.attribute(QStringLiteral("style"))))
				apply(style, d.first, d.second);
			return style;
		}

		/// Mean colour of a gradient: the best a part, which has a flat
		/// fill, can do.
		bool gradientColor(const QString &id, QColor *color, int depth = 0) const
		{
			const QDomElement g = m_ids.value(id);
			if (g.isNull() || depth > 8) return false;
			long r = 0, gr = 0, b = 0;
			int n = 0;
			for (QDomElement s = g.firstChildElement(); !s.isNull(); s = s.nextSiblingElement()) {
				if (localName(s) != QLatin1String("stop")) continue;
				QString value = s.attribute(QStringLiteral("stop-color"));
				for (const auto &d : parseDeclarations(s.attribute(QStringLiteral("style"))))
					if (d.first == QLatin1String("stop-color")) value = d.second;
				for (const CssRule &rule : m_rules)
					if (rule.selector == QLatin1Char('.') + s.attribute(QStringLiteral("class")))
						for (const auto &d : rule.declarations)
							if (d.first == QLatin1String("stop-color")) value = d.second;
				QColor c(Qt::black);
				if (!value.isEmpty() && !parseColor(value, &c)) continue;
				r += c.red(); gr += c.green(); b += c.blue();
				++n;
			}
			if (n) {
				*color = QColor(int(r / n), int(gr / n), int(b / n));
				return true;
			}
			QString href = g.attribute(QStringLiteral("xlink:href"));
			if (href.isEmpty()) href = g.attribute(QStringLiteral("href"));
			return href.startsWith(QLatin1Char('#'))
					&& gradientColor(href.mid(1), color, depth + 1);
		}

		/// The colour of a fill or a stroke, blended with white by its
		/// opacity. False: nothing is drawn.
		bool paint(const QString &text, double opacity, QColor *color) const
		{
			QColor c;
			const QString v = text.trimmed();
			if (v.startsWith(QLatin1String("url("))) {
				static const QRegularExpression rx(QStringLiteral("url\\(\\s*['\"]?#([^)'\" ]+)['\"]?\\s*\\)\\s*(.*)"));
				const auto m = rx.match(v);
				if (!m.hasMatch()) return false;
				if (!gradientColor(m.captured(1), &c) && !parseColor(m.captured(2), &c))
					c = Qt::black;
			} else if (!parseColor(v, &c)) {
				return false;
			}
			if (opacity < 0.05) return false;
			*color = blendWithWhite(c, opacity);
			return true;
		}

		static QRectF boundsOf(const QList<Sub> &subs)
		{
			QRectF r;
			bool first = true;
			for (const Sub &s : subs) {
				const QRectF b = s.path.boundingRect();
				r = first ? b : unite(r, b);
				first = false;
			}
			return r;
		}

		void add(Shape shape, const Style &style, const QTransform &tf)
		{
			if (shape.kind == Shape::Kind::Polygons) {
				shape.subs.erase(std::remove_if(shape.subs.begin(), shape.subs.end(),
					[](const Sub &s) { return s.path.elementCount() < 2; }), shape.subs.end());
				if (shape.subs.isEmpty()) return;
				for (Sub &s : shape.subs) s.path = tf.map(s.path);
				shape.bounds = boundsOf(shape.subs);
			} else {
				shape.bounds = shape.rect;
			}
			if (!qIsFinite(shape.bounds.width()) || !qIsFinite(shape.bounds.height())) return;
			shape.style = style;
			shape.scale = transformScale(tf);
			shapes << shape;
		}

		static bool axisAligned(const QTransform &t)
		{
			return std::abs(t.m12()) < 1e-9 && std::abs(t.m21()) < 1e-9;
		}

		static Sub closedSub(const QPainterPath &path)
		{
			Sub s;
			s.path = path;
			s.path.closeSubpath();
			s.closed = true;
			return s;
		}

		void shapeOf(const QDomElement &e, const QString &tag, const QTransform &tf, const Style &style)
		{
			auto attr = [&](const char *name) { return lengthValue(e.attribute(QLatin1String(name))); };
			Shape shape;
			if (tag == QLatin1String("rect")) {
				const double x = attr("x"), y = attr("y"), w = attr("width"), h = attr("height");
				if (w <= 0 || h <= 0) return;
				double rx = e.hasAttribute(QStringLiteral("rx")) ? attr("rx") : -1;
				double ry = e.hasAttribute(QStringLiteral("ry")) ? attr("ry") : -1;
				if (rx < 0) rx = ry < 0 ? 0 : ry;
				if (ry < 0) ry = rx;
				rx = qMin(rx, w / 2);
				ry = qMin(ry, h / 2);
				if (axisAligned(tf)) {
					shape.kind = Shape::Kind::Rectangle;
					shape.rect = tf.mapRect(QRectF(x, y, w, h));
					shape.rx = rx * std::abs(tf.m11());
					shape.ry = ry * std::abs(tf.m22());
				} else {
					QPainterPath p;
					if (rx > 0 && ry > 0) p.addRoundedRect(QRectF(x, y, w, h), rx, ry);
					else p.addRect(QRectF(x, y, w, h));
					shape.subs << closedSub(p);
				}
			} else if (tag == QLatin1String("circle") || tag == QLatin1String("ellipse")) {
				const double cx = attr("cx"), cy = attr("cy");
				double rx = attr("rx"), ry = attr("ry");
				if (tag == QLatin1String("circle")) rx = ry = attr("r");
				if (rx <= 0 || ry <= 0) return;
				const QRectF box(cx - rx, cy - ry, 2 * rx, 2 * ry);
				if (axisAligned(tf)) {
					shape.kind = Shape::Kind::Ellipse;
					shape.rect = tf.mapRect(box);
				} else {
					QPainterPath p;
					p.addEllipse(box);
					shape.subs << closedSub(p);
				}
			} else if (tag == QLatin1String("line")) {
				Sub s;
				s.path.moveTo(attr("x1"), attr("y1"));
				s.path.lineTo(attr("x2"), attr("y2"));
				shape.subs << s;
			} else if (tag == QLatin1String("polyline") || tag == QLatin1String("polygon")) {
				const QList<double> v = numberList(e.attribute(QStringLiteral("points")));
				if (v.size() < 4) return;
				Sub s;
				s.path.moveTo(v[0], v[1]);
				for (int i = 2; i + 1 < v.size(); i += 2) s.path.lineTo(v[i], v[i + 1]);
				if (tag == QLatin1String("polygon")) s = closedSub(s.path);
				shape.subs << s;
			} else if (tag == QLatin1String("path")) {
				shape.subs = parsePath(e.attribute(QStringLiteral("d")));
			} else {
				return;
			}
			add(shape, style, tf);
		}

		void walk(const QDomElement &e, const QTransform &parent, Style inherited, int depth,
				  bool in_marker = false)
		{
			if (depth > max_depth) return;
			const QString tag = localName(e);
			static const QStringList skipped = {
				QStringLiteral("defs"), QStringLiteral("clipPath"), QStringLiteral("mask"),
				QStringLiteral("symbol"), QStringLiteral("linearGradient"),
				QStringLiteral("radialGradient"), QStringLiteral("pattern"), QStringLiteral("style"),
				QStringLiteral("title"), QStringLiteral("desc"), QStringLiteral("metadata"),
				QStringLiteral("marker"), QStringLiteral("filter"), QStringLiteral("script"),
				QStringLiteral("namedview"), QStringLiteral("foreignObject")};
			if (skipped.contains(tag) && depth > 0) return;

			const Style style = styleOf(e, inherited);
			if (style.hidden) return;
			QTransform tf = parseTransform(e.attribute(QStringLiteral("transform"))) * parent;

			if (!in_marker && e.attribute(QStringLiteral("id")).startsWith(QLatin1String("terminal"),
																		  Qt::CaseInsensitive)) {
				markTerminal(e, tf, style, depth);
				return;
			}

			if (tag == QLatin1String("image")) { ++m_ignored_images; return; }
			if (tag == QLatin1String("text")) { ++m_ignored_texts; return; }

			if (tag == QLatin1String("use")) {
				QString href = e.attribute(QStringLiteral("xlink:href"));
				if (href.isEmpty()) href = e.attribute(QStringLiteral("href"));
				const QDomElement target = m_ids.value(href.mid(1));
				if (!href.startsWith(QLatin1Char('#')) || target.isNull()) return;
				tf = QTransform::fromTranslate(lengthValue(e.attribute(QStringLiteral("x"))),
											   lengthValue(e.attribute(QStringLiteral("y")))) * tf;
				const QString target_tag = localName(target);
				if (target_tag == QLatin1String("symbol") || target_tag == QLatin1String("svg")) {
					for (QDomElement c = target.firstChildElement(); !c.isNull(); c = c.nextSiblingElement())
						walk(c, tf, style, depth + 1, in_marker);
				} else {
					walk(target, tf, style, depth + 1, in_marker);
				}
				return;
			}

			if (tag == QLatin1String("svg") || tag == QLatin1String("g") || tag == QLatin1String("a")
				|| tag == QLatin1String("switch")) {
				for (QDomElement c = e.firstChildElement(); !c.isNull(); c = c.nextSiblingElement())
					walk(c, tf, style, depth + 1, in_marker);
				return;
			}
			shapeOf(e, tag, tf, style);
		}

		void markTerminal(const QDomElement &e, const QTransform &tf, const Style &style, int depth)
		{
			const QList<Shape> saved = shapes;
			shapes.clear();
			// the element itself, with the id that marks it taken as read
			const QString tag = localName(e);
			if (tag == QLatin1String("g") || tag == QLatin1String("a") || tag == QLatin1String("svg")) {
				for (QDomElement c = e.firstChildElement(); !c.isNull(); c = c.nextSiblingElement())
					walk(c, tf, style, depth + 1, true);
			} else {
				shapeOf(e, tag, tf, style);
			}
			QRectF box;
			bool first = true;
			for (const Shape &s : std::as_const(shapes)) {
				box = first ? s.bounds : unite(box, s.bounds);
				first = false;
			}
			shapes = saved;
			if (first) return;

			SvgToElmt::Terminal t;
			QString name = e.attribute(QStringLiteral("id")).mid(int(qstrlen("terminal")));
			name.remove(QRegularExpression(QStringLiteral("^[\\s_:.-]+")));
			t.name = name;
			t.position = box.center();
			const QString o = e.attribute(QStringLiteral("data-orientation")).trimmed().toLower();
			if (o.size() == 1 && QStringLiteral("nesw").contains(o)) t.orientation = o;
			markers << t;
		}

		QDomElement m_root;
		SvgToElmt::Result *m_result;
		QHash<QString, QDomElement> m_ids;
		QList<CssRule> m_rules;
		int m_ignored_images = 0;
		int m_ignored_texts = 0;
};

QString styleString(const Shape &shape, bool fill_visible, const QString &fill_name,
					bool stroke_visible, const QString &stroke_name)
{
	QString weight = QStringLiteral("none");
	if (stroke_visible) {
		const double w = shape.style.strokeWidth * shape.scale;
		weight = w < 0.3 ? QStringLiteral("thin")
				 : w < 1.5 ? QStringLiteral("normal")
				 : w < 3.5 ? QStringLiteral("hight") : QStringLiteral("eleve");
	}
	return QStringLiteral("line-style:normal;line-weight:%1;filling:%2;color:%3")
			.arg(weight, fill_visible ? fill_name : QStringLiteral("none"),
				 stroke_visible ? stroke_name : QStringLiteral("black"));
}

QString orientationOf(const QPointF &p, const QRectF &box)
{
	const double left = std::abs(p.x() - box.left()), right = std::abs(box.right() - p.x());
	const double top = std::abs(p.y() - box.top()), bottom = std::abs(box.bottom() - p.y());
	const double best = std::min({left, right, top, bottom});
	if (best == top) return QStringLiteral("n");
	if (best == bottom) return QStringLiteral("s");
	if (best == left) return QStringLiteral("w");
	return QStringLiteral("e");
}

} // namespace

namespace SvgToElmt {

Result convert(const QByteArray &svg, const Options &options)
{
	Result result;

	QDomDocument doc;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	if (const auto parsed = doc.setContent(svg); !parsed) {
		result.error = QStringLiteral("not an XML file (line %1: %2)")
						.arg(parsed.errorLine).arg(parsed.errorMessage);
		return result;
	}
#else
	QString error;
	int line = 0;
	if (!doc.setContent(svg, &error, &line)) {
		result.error = QStringLiteral("not an XML file (line %1: %2)").arg(line).arg(error);
		return result;
	}
#endif
	const QDomElement root = doc.documentElement();
	if (localName(root) != QLatin1String("svg")) {
		result.error = QStringLiteral("not a SVG file (root element <%1>)").arg(root.tagName());
		return result;
	}

	Converter converter(root, &result);
	converter.run();
	const QList<Shape> &shapes = converter.shapes;
	if (shapes.isEmpty()) {
		result.error = QStringLiteral("nothing to draw in the SVG");
		return result;
	}

	QRectF bounds = shapes.first().bounds;
	for (const Shape &s : shapes) bounds = unite(bounds, s.bounds);

	double scale = 1;
	if (options.scale > 0) scale = options.scale;
	else if (options.width > 0 && bounds.width() > 0) scale = options.width / bounds.width();
	else if (options.height > 0 && bounds.height() > 0) scale = options.height / bounds.height();
	else if (options.widthInCells > 0 && bounds.width() > 0)
		scale = options.widthInCells * grid_step / bounds.width();
	else if (options.pixelsPerMm > 0 && options.unitMm > 0)
		scale = options.pixelsPerMm * options.unitMm;
	if (!std::isfinite(scale) || scale <= 0) {
		result.error = QStringLiteral("cannot scale the drawing (%1 x %2)")
						.arg(bounds.width()).arg(bounds.height());
		return result;
	}

	// The insertion point is the centre of the drawing.
	QTransform to_element;
	to_element.scale(scale, scale);
	to_element.translate(-bounds.center().x(), -bounds.center().y());
	const QRectF drawing = to_element.mapRect(bounds);

	QList<Terminal> terminals = converter.markers;
	terminals << options.terminals;
	QRectF extent = drawing;
	for (Terminal &t : terminals) {
		t.position = to_element.map(t.position);
		if (options.snapTerminals)
			t.position = QPointF(qRound(t.position.x() / grid_step) * grid_step,
								 qRound(t.position.y() / grid_step) * grid_step);
		extent = unite(extent, QRectF(t.position, QSizeF(0, 0)));
		if (t.orientation.isEmpty()) t.orientation = orientationOf(t.position, drawing);
	}
	auto up = [](double v) { return qMax(grid_step, int(std::ceil(v / grid_step - 1e-6)) * grid_step); };
	const int half_width = up(qMax(std::abs(extent.left()), std::abs(extent.right())));
	const int half_height = up(qMax(std::abs(extent.top()), std::abs(extent.bottom())));
	if (2 * half_width > 600 || 2 * half_height > 600)
		result.warnings << QStringLiteral("the element is %1 x %2 pixels: use --mm-scale, --cells, --width, --height or --scale")
							.arg(2 * half_width).arg(2 * half_height);

	QDomDocument out;
	QDomElement definition = out.createElement(QStringLiteral("definition"));
	definition.setAttribute(QStringLiteral("version"), QStringLiteral("0.100.0"));
	definition.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	definition.setAttribute(QStringLiteral("link_type"), QStringLiteral("simple"));
	definition.setAttribute(QStringLiteral("width"), 2 * half_width);
	definition.setAttribute(QStringLiteral("height"), 2 * half_height);
	definition.setAttribute(QStringLiteral("hotspot_x"), half_width);
	definition.setAttribute(QStringLiteral("hotspot_y"), half_height);
	out.appendChild(definition);

	QDomElement uuid = out.createElement(QStringLiteral("uuid"));
	uuid.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
	definition.appendChild(uuid);

	QDomElement names = out.createElement(QStringLiteral("names"));
	QDomElement name = out.createElement(QStringLiteral("name"));
	name.setAttribute(QStringLiteral("lang"), options.language.isEmpty() ? QStringLiteral("en") : options.language);
	name.appendChild(out.createTextNode(options.name.isEmpty() ? QStringLiteral("SVG element") : options.name));
	names.appendChild(name);
	definition.appendChild(names);

	QDomElement description = out.createElement(QStringLiteral("description"));
	definition.appendChild(description);

	auto point = [&](QDomElement &e, const char *x, const char *y, QPointF p) {
		e.setAttribute(QLatin1String(x), number(p.x()));
		e.setAttribute(QLatin1String(y), number(p.y()));
	};

	for (const Shape &shape : shapes) {
		QColor fill_color, stroke_color;
		const bool fill_visible = converter.paintOf(shape.style.fill,
													shape.style.fillOpacity, &fill_color);
		const bool stroke_visible = converter.paintOf(shape.style.stroke,
													  shape.style.strokeOpacity, &stroke_color);
		if (!fill_visible && !stroke_visible) continue;
		const QString style = styleString(shape, fill_visible,
										  nearestName(fill_color),
										  stroke_visible, nearestName(stroke_color));
		auto finish = [&](QDomElement &e) {
			e.setAttribute(QStringLiteral("style"), style);
			e.setAttribute(QStringLiteral("antialias"), QStringLiteral("true"));
			description.appendChild(e);
			++result.parts;
		};

		if (shape.kind == Shape::Kind::Rectangle) {
			const QRectF r = to_element.mapRect(shape.rect);
			QDomElement e = out.createElement(QStringLiteral("rect"));
			point(e, "x", "y", r.topLeft());
			e.setAttribute(QStringLiteral("width"), number(r.width()));
			e.setAttribute(QStringLiteral("height"), number(r.height()));
			e.setAttribute(QStringLiteral("rx"), number(shape.rx * scale));
			e.setAttribute(QStringLiteral("ry"), number(shape.ry * scale));
			finish(e);
		} else if (shape.kind == Shape::Kind::Ellipse) {
			const QRectF r = to_element.mapRect(shape.rect);
			QDomElement e;
			if (qFuzzyCompare(r.width() + 1, r.height() + 1)) {
				e = out.createElement(QStringLiteral("circle"));
				e.setAttribute(QStringLiteral("diameter"), number(r.width()));
			} else {
				e = out.createElement(QStringLiteral("ellipse"));
				e.setAttribute(QStringLiteral("width"), number(r.width()));
				e.setAttribute(QStringLiteral("height"), number(r.height()));
			}
			point(e, "x", "y", r.topLeft());
			finish(e);
		} else {
			for (const Sub &sub : shape.subs) {
				const QList<QPolygonF> polygons = to_element.map(sub.path).toSubpathPolygons();
				for (QPolygonF polygon : polygons) {
					if (sub.closed && polygon.size() > 1 && polygon.first() == polygon.last())
						polygon.removeLast();
					if (polygon.size() < 2) continue;
					// A stroke-only 2-point path is a line.
					if (!sub.closed && polygon.size() == 2 && !fill_visible) {
						QDomElement e = out.createElement(QStringLiteral("line"));
						point(e, "x1", "y1", polygon.at(0));
						point(e, "x2", "y2", polygon.at(1));
						e.setAttribute(QStringLiteral("end1"), QStringLiteral("none"));
						e.setAttribute(QStringLiteral("length1"), QStringLiteral("1.5"));
						e.setAttribute(QStringLiteral("end2"), QStringLiteral("none"));
						e.setAttribute(QStringLiteral("length2"), QStringLiteral("1.5"));
						finish(e);
						continue;
					}
					if (polygon.size() < 3 && fill_visible && !stroke_visible) continue;
					QDomElement e = out.createElement(QStringLiteral("polygon"));
					for (int i = 0; i < polygon.size(); ++i)
						point(e, qPrintable(QStringLiteral("x%1").arg(i + 1)),
							  qPrintable(QStringLiteral("y%1").arg(i + 1)), polygon.at(i));
					if (!sub.closed)
						e.setAttribute(QStringLiteral("closed"), QStringLiteral("false"));
					finish(e);
				}
			}
		}
	}
	if (!result.parts) {
		result.error = QStringLiteral("nothing visible to draw in the SVG");
		return result;
	}

	QStringList used;
	for (const Terminal &t : std::as_const(terminals)) {
		QString terminal_name = t.name;
		if (terminal_name.isEmpty()) {
			terminal_name = QString::number(result.terminals + 1);
			result.warnings << QStringLiteral("terminal at %1,%2 has no name: \"%3\" used")
								.arg(number(t.position.x())).arg(number(t.position.y())).arg(terminal_name);
		}
		if (used.contains(terminal_name))
			result.warnings << QStringLiteral("terminal name \"%1\" is used twice").arg(terminal_name);
		used << terminal_name;
		QDomElement e = out.createElement(QStringLiteral("terminal"));
		e.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
		e.setAttribute(QStringLiteral("name"), terminal_name);
		point(e, "x", "y", t.position);
		e.setAttribute(QStringLiteral("orientation"), t.orientation);
		e.setAttribute(QStringLiteral("type"), QStringLiteral("Generic"));
		description.appendChild(e);
		++result.terminals;
	}
	if (!result.terminals && options.warnNoTerminal)
		result.warnings << QStringLiteral(
			"no terminal: add some with --terminal or an id=\"terminal-...\" "
			"shape");

	result.definition = QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
						+ out.toByteArray(4);
	result.ok = true;
	return result;
}

Result convertImage(const QImage &image, Options options,
					const RasterVectorizer::Options &vectorizer, double dpi)
{
	Result result;
	const RasterVectorizer::Result traced = RasterVectorizer::toSvg(image, vectorizer);
	if (!traced.ok) {
		result.error = traced.error;
		return result;
	}
	if (dpi > 0) options.unitMm = 25.4 / dpi;
	result = convert(traced.svg, options);
	result.warnings = traced.warnings + result.warnings;
	return result;
}

namespace {

const QStringList &pictureSuffixes()
{
	static const QStringList list{QStringLiteral("png"), QStringLiteral("jpg"),
								  QStringLiteral("jpeg"), QStringLiteral("bmp"),
								  QStringLiteral("gif"), QStringLiteral("webp")};
	return list;
}

bool looksLikeSvg(const QByteArray &data)
{
	return data.left(4096).contains("<svg");
}

} // namespace

Result convertFile(const QString &path, const Options &options,
				   const RasterVectorizer::Options &vectorizer, double dpi)
{
	Result result;
	const QString suffix = QFileInfo(path).suffix().toLower();
	if (pictureSuffixes().contains(suffix)) {
		QImage image;
		if (!image.load(path)) {
			result.error = QStringLiteral("cannot read the picture");
			return result;
		}
		return convertImage(image, options, vectorizer, dpi);
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		result.error = QStringLiteral("cannot read the file");
		return result;
	}
	return convert(file.readAll(), options);
}

QString fileFilter()
{
	return QStringLiteral("SVG, PNG, JPEG, BMP, GIF (*.svg *.png *.jpg *.jpeg *.bmp *.gif *.webp)");
}

bool sourceFromMimeData(const QMimeData *mime, QByteArray *svg,
						QImage *image, QString *name)
{
	if (!mime) return false;
	svg->clear();
	*image = QImage();
	const QString svg_type = QStringLiteral("image/svg+xml");
	if (mime->hasFormat(svg_type) && looksLikeSvg(mime->data(svg_type))) {
		*svg = mime->data(svg_type);
		return true;
	}
	for (const QUrl &url : mime->urls()) {
		if (!url.isLocalFile()) continue;
		const QString path = url.toLocalFile();
		const QString suffix = QFileInfo(path).suffix().toLower();
		if (suffix == QLatin1String("svg")) {
			QFile file(path);
			if (file.open(QIODevice::ReadOnly) && looksLikeSvg(file.read(4096))) {
				file.seek(0);
				*svg = file.readAll();
				if (name) *name = QFileInfo(path).completeBaseName();
				return true;
			}
		} else if (pictureSuffixes().contains(suffix) && image->load(path)) {
			if (name) *name = QFileInfo(path).completeBaseName();
			return true;
		}
	}
	if (mime->hasImage()) {
		*image = qvariant_cast<QImage>(mime->imageData());
		if (!image->isNull()) return true;
	}
	if (mime->hasText()) {
		const QByteArray text = mime->text().toUtf8();
		if (looksLikeSvg(text)) {
			*svg = text;
			return true;
		}
	}
	return false;
}

bool parseTerminal(const QString &text, Terminal *terminal)
{
	const QStringList parts = text.split(QLatin1Char(','));
	if (parts.size() < 2 || parts.size() > 4) return false;
	bool ok_x = false, ok_y = false;
	const double x = parts.at(0).trimmed().toDouble(&ok_x);
	const double y = parts.at(1).trimmed().toDouble(&ok_y);
	if (!ok_x || !ok_y) return false;
	QString orientation = parts.value(2).trimmed().toLower();
	if (!orientation.isEmpty()
		&& (orientation.size() != 1 || !QStringLiteral("nesw").contains(orientation)))
		return false;
	terminal->position = QPointF(x, y);
	terminal->orientation = orientation;
	terminal->name = parts.value(3).trimmed();
	return true;
}

QList<Terminal> parseTerminals(const QString &text, QStringList *errors)
{
	QList<Terminal> list;
	const QStringList lines = text.split(QLatin1Char('\n'));
	static const QRegularExpression separator(QStringLiteral("[;,\\t]"));
	for (int i = 0; i < lines.size(); ++i) {
		const QString line = lines.at(i).trimmed();
		if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
		const QStringList f = line.split(separator);
		bool ok_x = false, ok_y = false;
		const double x = f.value(1).trimmed().toDouble(&ok_x);
		const double y = f.value(2).trimmed().toDouble(&ok_y);
		const QString o = f.value(3).trimmed().toLower();
		const bool good_o = o.isEmpty() || (o.size() == 1 && QStringLiteral("nesw").contains(o));
		if (!ok_x || !ok_y || !good_o) {
			// the header line has no number
			if (!list.isEmpty() || i > 0 || (ok_x || ok_y))
				if (errors) *errors << QStringLiteral("line %1: %2").arg(i + 1).arg(line);
			continue;
		}
		Terminal t;
		t.name = f.at(0).trimmed();
		t.position = QPointF(x, y);
		t.orientation = o;
		list << t;
	}
	return list;
}

int run(const QStringList &args)
{
	QTextStream err(stderr);
	const QString usage = QStringLiteral(
		"usage: --svg-to-elmt <input.svg|input.png|https://address>\n"
		"       <output.elmt>\n"
		"       [--name <text>] [--lang <code>]\n"
		"       [--mm-scale <px per mm> | --cells <width in grid cells>\n"
		"        | --width <px> | --height <px> | --scale <factor>]\n"
		"       [--no-snap] [--dpi <n>]\n"
		"       [--threshold <0-255>] [--invert] [--simplify <px>]\n"
		"       [--despeckle <px>] [--max-paths <n>]\n"
		"       [--terminal x,y[,n|e|s|w[,name]]]... [--terminals <file.csv>]\n");
	QStringList files;
	Options options;
	RasterVectorizer::Options vectorizer;
	double dpi = 96;
	double mm_scale = 0;

	for (int i = 0; i < args.size(); ++i) {
		const QString &a = args.at(i);
		if (!a.startsWith(QLatin1String("--"))) { files << a; continue; }
		if (a == QLatin1String("--invert")) { vectorizer.invert = true; continue; }
		if (a == QLatin1String("--no-snap")) { options.snapTerminals = false; continue; }
		if (i + 1 >= args.size()) { err << a << " needs a value\n" << usage; return 2; }
		const QString &v = args.at(++i);
		bool ok = true;
		if (a == QLatin1String("--name")) options.name = v;
		else if (a == QLatin1String("--lang")) options.language = v;
		else if (a == QLatin1String("--scale")) options.scale = v.toDouble(&ok);
		else if (a == QLatin1String("--width")) options.width = v.toDouble(&ok);
		else if (a == QLatin1String("--height")) options.height = v.toDouble(&ok);
		else if (a == QLatin1String("--cells")) options.widthInCells = v.toDouble(&ok);
		else if (a == QLatin1String("--mm-scale")) mm_scale = v.toDouble(&ok);
		else if (a == QLatin1String("--dpi")) dpi = v.toDouble(&ok);
		else if (a == QLatin1String("--threshold")) vectorizer.threshold = v.toInt(&ok);
		else if (a == QLatin1String("--simplify")) vectorizer.simplify = v.toDouble(&ok);
		else if (a == QLatin1String("--despeckle")) vectorizer.despeckle = v.toInt(&ok);
		else if (a == QLatin1String("--max-paths")) vectorizer.maxPaths = v.toInt(&ok);
		else if (a == QLatin1String("--terminal")) {
			Terminal t;
			if ((ok = parseTerminal(v, &t))) options.terminals << t;
		} else if (a == QLatin1String("--terminals")) {
			QFile f(v);
			if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
				err << "cannot read " << v << "\n";
				return 2;
			}
			QTextStream in(&f);
			QStringList bad;
			options.terminals << parseTerminals(in.readAll(), &bad);
			for (const QString &b : bad) err << v << ": " << b << "\n";
			if (!bad.isEmpty()) return 2;
		} else {
			err << "unknown option " << a << "\n" << usage;
			return 2;
		}
		if (!ok) { err << "bad value for " << a << ": " << v << "\n"; return 2; }
	}
	if (files.size() != 2) { err << usage; return 2; }
	if (dpi <= 0 || vectorizer.threshold < 0 || vectorizer.threshold > 255) {
		err << "bad --dpi or --threshold\n";
		return 2;
	}

	// Without a size, a millimetre of the drawing is 2 element pixels.
	options.pixelsPerMm = mm_scale;
	if (!options.scale && !options.width && !options.height
		&& !options.widthInCells && !mm_scale)
		options.pixelsPerMm = 2;
	if (options.name.isEmpty())
		options.name = QFileInfo(files.at(1)).completeBaseName();

	QString input = files.at(0);
	const bool from_web = PictureUrl::isUrl(input);
	if (from_web) {
		QString picture_file, problem;
		QUrl picture_url;
		if (!PictureUrl::download(input, &picture_file, &picture_url,
								  &problem)) {
			err << input << ": " << problem << "\n";
			return 1;
		}
		err << "picture: " << picture_url.toString() << "\n"
			<< PictureUrl::notice() << "\n";
		input = picture_file;
	}

	const Result result = convertFile(input, options, vectorizer, dpi);
	for (const QString &w : result.warnings) err << "warning: " << w << "\n";
	if (!result.ok) {
		err << input << ": " << result.error << "\n";
		return 1;
	}
	if (from_web && PictureUrl::isInGitRepository(files.at(1))) {
		err << "warning: " << files.at(1)
			<< " is in a git repository: do not commit it\n";
	}

	QDir().mkpath(QFileInfo(files.at(1)).absolutePath());
	QSaveFile out(files.at(1));
	if (!out.open(QIODevice::WriteOnly) || out.write(result.definition) < 0 || !out.commit()) {
		err << "cannot write " << files.at(1) << "\n";
		return 1;
	}
	QTextStream(stdout) << files.at(1) << ": " << result.parts << " part(s), "
						<< result.terminals << " terminal(s)\n";
	return 0;
}

} // namespace SvgToElmt

