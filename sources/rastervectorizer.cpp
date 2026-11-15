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
#include "rastervectorizer.h"

#include <QList>
#include <QPointF>
#include <QRegularExpression>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace {

constexpr qint64 max_pixels = 36000000;

/// Directions along the edges of the pixels, y pointing down. The drawn
/// pixel is on the right of an edge, so the outline of an area goes
/// clockwise on the screen, and that of a hole the other way.
enum Direction { East = 0, South, West, North };
constexpr int step_x[4] = {1, 0, -1, 0};
constexpr int step_y[4] = {0, 1, 0, -1};

struct Loop {
	QVector<QPointF> points;
	/// Surface in pixels, negative for a hole
	double area = 0;
};

double signedArea(const QVector<QPointF> &points)
{
	double sum = 0;
	for (int i = 0; i < points.size(); ++i) {
		const QPointF &a = points.at(i);
		const QPointF &b = points.at((i + 1) % points.size());
		sum += a.x() * b.y() - b.x() * a.y();
	}
	return sum / 2;
}

double distanceToSegment(const QPointF &p, const QPointF &a, const QPointF &b)
{
	const double dx = b.x() - a.x();
	const double dy = b.y() - a.y();
	const double length2 = dx * dx + dy * dy;
	double t = 0;
	if (length2 > 0)
		t = qBound(0.0, ((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / length2, 1.0);
	return std::hypot(p.x() - (a.x() + t * dx), p.y() - (a.y() + t * dy));
}

/// Douglas-Peucker on the open polyline @p line, ends kept.
QVector<QPointF> simplifyLine(const QVector<QPointF> &line, double tolerance)
{
	const int count = line.size();
	if (count < 3) return line;
	QVector<bool> keep(count, false);
	keep[0] = keep[count - 1] = true;
	QList<QPair<int, int>> pending;
	pending << qMakePair(0, count - 1);
	while (!pending.isEmpty()) {
		const auto [first, last] = pending.takeLast();
		double farthest = 0;
		int index = -1;
		for (int i = first + 1; i < last; ++i) {
			const double d = distanceToSegment(line.at(i), line.at(first), line.at(last));
			if (d > farthest) { farthest = d; index = i; }
		}
		if (index >= 0 && farthest > tolerance) {
			keep[index] = true;
			pending << qMakePair(first, index) << qMakePair(index, last);
		}
	}
	QVector<QPointF> result;
	for (int i = 0; i < count; ++i)
		if (keep.at(i)) result << line.at(i);
	return result;
}

/// Simplify the closed outline @p ring: it is cut in two at its first
/// point and at the one farthest from it.
QVector<QPointF> simplifyRing(const QVector<QPointF> &ring, double tolerance)
{
	if (tolerance <= 0 || ring.size() < 4) return ring;
	int far = 1;
	double farthest = -1;
	for (int i = 1; i < ring.size(); ++i) {
		const double d = std::hypot(ring.at(i).x() - ring.at(0).x(),
									ring.at(i).y() - ring.at(0).y());
		if (d > farthest) { farthest = d; far = i; }
	}
	QVector<QPointF> first_half = ring.mid(0, far + 1);
	QVector<QPointF> second_half = ring.mid(far);
	second_half << ring.at(0);
	QVector<QPointF> result = simplifyLine(first_half, tolerance);
	result.removeLast();
	const QVector<QPointF> second = simplifyLine(second_half, tolerance);
	for (int i = 0; i < second.size() - 1; ++i) result << second.at(i);
	return result;
}

QString numberText(double value)
{
	return QString::number(value, 'f', 2).remove(QRegularExpression(QStringLiteral("\\.?0+$")));
}

} // namespace

namespace RasterVectorizer {

Result toSvg(const QImage &image, const Options &options)
{
	Result result;
	if (image.isNull()) {
		result.error = QStringLiteral("the picture is empty");
		return result;
	}
	const int width = image.width();
	const int height = image.height();
	if (qint64(width) * height > max_pixels) {
		result.error = QStringLiteral("the picture is too large (%1 x %2 pixels)")
						.arg(width).arg(height);
		return result;
	}

	// Drawn pixels
	const QImage source = image.convertToFormat(QImage::Format_ARGB32);
	QVector<quint8> drawn(qsizetype(width) * height, 0);
	for (int y = 0; y < height; ++y) {
		const QRgb *line = reinterpret_cast<const QRgb *>(source.constScanLine(y));
		for (int x = 0; x < width; ++x) {
			const int alpha = qAlpha(line[x]);
			const int gray = qGray(line[x]);
			bool dark;
			if (options.invert)
				dark = alpha * gray / 255 >= options.threshold;
			else
				dark = 255 - alpha * (255 - gray) / 255 < options.threshold;
			drawn[qsizetype(y) * width + x] = dark;
		}
	}
	auto is_drawn = [&](int x, int y) {
		return x >= 0 && y >= 0 && x < width && y < height
			   && drawn.at(qsizetype(y) * width + x);
	};

	// The edges between a drawn pixel and a light one, by the corner
	// they leave: one bit per direction.
	const int stride = width + 1;
	QVector<quint8> edges(qsizetype(stride) * (height + 1), 0);
	auto add_edge = [&](int x, int y, Direction direction) {
		edges[qsizetype(y) * stride + x] |= quint8(1 << direction);
	};
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (!is_drawn(x, y)) continue;
			if (!is_drawn(x, y - 1)) add_edge(x, y, East);
			if (!is_drawn(x + 1, y)) add_edge(x + 1, y, South);
			if (!is_drawn(x, y + 1)) add_edge(x + 1, y + 1, West);
			if (!is_drawn(x - 1, y)) add_edge(x, y + 1, North);
		}
	}

	// Follow the edges. Where two outlines touch at a corner, each edge
	// goes on to the first of: right, straight, left. That is a pairing
	// of the edges, so every outline closes on its first edge.
	QVector<quint8> used(edges.size(), 0);
	QList<Loop> loops;
	for (int y = 0; y <= height; ++y) {
		for (int x = 0; x <= width; ++x) {
			const qsizetype start = qsizetype(y) * stride + x;
			for (int first = 0; first < 4; ++first) {
				if (!(edges.at(start) & (1 << first)) || (used.at(start) & (1 << first)))
					continue;
				Loop loop;
				int cx = x, cy = y, direction = first;
				do {
					used[qsizetype(cy) * stride + cx] |= quint8(1 << direction);
					cx += step_x[direction];
					cy += step_y[direction];
					const quint8 mask = edges.at(qsizetype(cy) * stride + cx);
					int next = -1;
					for (int turn : {1, 0, 3}) {
						const int candidate = (direction + turn) % 4;
						if (mask & (1 << candidate)) { next = candidate; break; }
					}
					if (next < 0) break;
					if (next != direction) loop.points << QPointF(cx, cy);
					direction = next;
				} while (!(cx == x && cy == y && direction == first));
				if (loop.points.size() < 3) continue;
				loop.area = signedArea(loop.points);
				loops << loop;
			}
		}
	}

	// A tiny area, or a one-pixel hole, is noise.
	loops.erase(std::remove_if(loops.begin(), loops.end(), [&](const Loop &l) {
		return std::abs(l.area) <= options.despeckle;
	}), loops.end());
	// The largest first: it contains the smaller ones it is drawn under.
	std::stable_sort(loops.begin(), loops.end(), [](const Loop &a, const Loop &b) {
		return std::abs(a.area) > std::abs(b.area);
	});
	if (options.maxPaths > 0 && loops.size() > options.maxPaths) {
		result.warnings << QStringLiteral("%1 outlines found, only the %2 largest are kept")
							.arg(loops.size()).arg(options.maxPaths);
		loops = loops.mid(0, options.maxPaths);
	}

	QByteArray body;
	for (const Loop &loop : std::as_const(loops)) {
		const QVector<QPointF> points = simplifyRing(loop.points, options.simplify);
		if (points.size() < 3) continue;
		QString d;
		for (int i = 0; i < points.size(); ++i)
			d += QStringLiteral("%1%2 %3 ")
					.arg(i ? QLatin1String("L") : QLatin1String("M"))
					.arg(numberText(points.at(i).x()))
					.arg(numberText(points.at(i).y()));
		d += QLatin1Char('Z');
		body += QStringLiteral("<path fill=\"%1\" stroke=\"none\" d=\"%2\"/>\n")
					.arg(loop.area > 0 ? QLatin1String("black") : QLatin1String("white"))
					.arg(d).toUtf8();
		++result.paths;
	}
	if (!result.paths) {
		result.error = options.invert
			? QStringLiteral("nothing to draw: no pixel is lighter than %1").arg(options.threshold)
			: QStringLiteral("nothing to draw: no pixel is darker than %1").arg(options.threshold);
		return result;
	}

	result.svg = QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" "
								"width=\"%1\" height=\"%2\" viewBox=\"0 0 %1 %2\">\n")
					 .arg(width).arg(height).toUtf8()
				 + body + "</svg>\n";
	result.ok = true;
	return result;
}

} // namespace RasterVectorizer
