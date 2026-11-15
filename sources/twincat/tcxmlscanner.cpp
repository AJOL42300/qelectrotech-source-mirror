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
#include "tcxmlscanner.h"

namespace {
bool isSpace(QChar c)
{
	return c == QLatin1Char(' ') || c == QLatin1Char('\t')
		|| c == QLatin1Char('\r') || c == QLatin1Char('\n');
}
}

/**
 * @brief TcXmlScanner::parse
 * @param text Whole document, already decoded (no byte-order mark).
 * @return true if the document is well formed enough to be patched.
 */
bool TcXmlScanner::parse(const QString &text)
{
	m_text = text;
	m_error.clear();
	m_nodes.clear();
	m_root = -1;

	const int n = text.size();
	int pos = 0;
	QVector<int> stack;

	auto fail = [this, &pos](const QString &what) {
		m_error = QStringLiteral("%1 at offset %2").arg(what).arg(pos);
		return false;
	};

	while (pos < n) {
		if (text.at(pos) != QLatin1Char('<')) {
			++pos;
			continue;
		}
		if (text.mid(pos, 4) == QLatin1String("<!--")) {
			const int end = text.indexOf(QLatin1String("-->"), pos + 4);
			if (end < 0) return fail(QStringLiteral("Unterminated comment"));
			pos = end + 3;
			continue;
		}
		if (text.mid(pos, 9) == QLatin1String("<![CDATA[")) {
			const int end = text.indexOf(QLatin1String("]]>"), pos + 9);
			if (end < 0) return fail(QStringLiteral("Unterminated CDATA"));
			pos = end + 3;
			continue;
		}
		if (text.mid(pos, 2) == QLatin1String("<?")) {
			const int end = text.indexOf(QLatin1String("?>"), pos + 2);
			if (end < 0) return fail(QStringLiteral("Unterminated declaration"));
			pos = end + 2;
			continue;
		}
		if (text.mid(pos, 2) == QLatin1String("<!")) {
			int depth = 0;
			int i = pos + 2;
			for (; i < n; ++i) {
				const QChar c = text.at(i);
				if (c == QLatin1Char('[')) ++depth;
				else if (c == QLatin1Char(']')) --depth;
				else if (c == QLatin1Char('>') && depth <= 0) break;
			}
			if (i >= n) return fail(QStringLiteral("Unterminated DOCTYPE"));
			pos = i + 1;
			continue;
		}
		if (text.mid(pos, 2) == QLatin1String("</")) {
			if (stack.isEmpty()) return fail(QStringLiteral("Unexpected end tag"));
			const int end = text.indexOf(QLatin1Char('>'), pos + 2);
			if (end < 0) return fail(QStringLiteral("Unterminated end tag"));
			const QString name = text.mid(pos + 2, end - pos - 2).trimmed();
			Node &node = m_nodes[stack.last()];
			if (name != node.name) {
				return fail(QStringLiteral("Mismatched end tag </%1> for <%2>")
							.arg(name, node.name));
			}
			node.contentEnd = pos;
			node.endEnd = end + 1;
			stack.removeLast();
			pos = end + 1;
			continue;
		}

		// Start tag
		Node node;
		node.startBegin = pos;
		node.parent = stack.isEmpty() ? -1 : stack.last();
		int i = pos + 1;
		const int nameBegin = i;
		while (i < n && !isSpace(text.at(i))
			   && text.at(i) != QLatin1Char('>')
			   && text.at(i) != QLatin1Char('/')) ++i;
		node.name = text.mid(nameBegin, i - nameBegin);
		if (node.name.isEmpty()) return fail(QStringLiteral("Empty tag name"));

		bool closed = false;
		while (i < n) {
			while (i < n && isSpace(text.at(i))) ++i;
			if (i >= n) break;
			const QChar c = text.at(i);
			if (c == QLatin1Char('>')) {
				++i;
				closed = true;
				break;
			}
			if (c == QLatin1Char('/')) {
				if (i + 1 < n && text.at(i + 1) == QLatin1Char('>')) {
					node.selfClosing = true;
					i += 2;
					closed = true;
					break;
				}
				pos = i;
				return fail(QStringLiteral("Unexpected '/' in tag"));
			}
			Attribute attr;
			const int attrBegin = i;
			while (i < n && !isSpace(text.at(i))
				   && text.at(i) != QLatin1Char('=')
				   && text.at(i) != QLatin1Char('>')) ++i;
			attr.name = text.mid(attrBegin, i - attrBegin);
			while (i < n && isSpace(text.at(i))) ++i;
			if (i >= n || text.at(i) != QLatin1Char('=')) {
				pos = i;
				return fail(QStringLiteral("Attribute without value"));
			}
			++i;
			while (i < n && isSpace(text.at(i))) ++i;
			if (i >= n || (text.at(i) != QLatin1Char('"')
						   && text.at(i) != QLatin1Char('\''))) {
				pos = i;
				return fail(QStringLiteral("Attribute value not quoted"));
			}
			const QChar quote = text.at(i);
			const int valueEnd = text.indexOf(quote, i + 1);
			if (valueEnd < 0) {
				pos = i;
				return fail(QStringLiteral("Unterminated attribute value"));
			}
			attr.rawBegin = i + 1;
			attr.rawEnd = valueEnd;
			attr.value = decodeEntities(
				text.mid(attr.rawBegin, attr.rawEnd - attr.rawBegin));
			node.attributes.append(attr);
			i = valueEnd + 1;
		}
		if (!closed) return fail(QStringLiteral("Unterminated start tag"));

		node.startEnd = i;
		node.contentBegin = i;
		node.contentEnd = i;
		node.endEnd = i;

		const int index = m_nodes.size();
		if (node.parent >= 0) {
			m_nodes[node.parent].children.append(index);
		} else {
			if (m_root >= 0) return fail(QStringLiteral("Several root elements"));
			m_root = index;
		}
		m_nodes.append(node);
		if (!node.selfClosing) stack.append(index);
		pos = i;
	}

	if (!stack.isEmpty()) {
		return fail(QStringLiteral("Unclosed element <%1>")
					.arg(m_nodes.at(stack.last()).name));
	}
	if (m_root < 0) return fail(QStringLiteral("No root element"));
	return true;
}

int TcXmlScanner::child(int node, const QString &name) const
{
	for (int c : m_nodes.at(node).children) {
		if (m_nodes.at(c).name == name) return c;
	}
	return -1;
}

QVector<int> TcXmlScanner::childrenNamed(int node, const QString &name) const
{
	QVector<int> result;
	for (int c : m_nodes.at(node).children) {
		if (m_nodes.at(c).name == name) result.append(c);
	}
	return result;
}

int TcXmlScanner::attributeIndex(int node, const QString &name) const
{
	const auto &attrs = m_nodes.at(node).attributes;
	for (int i = 0; i < attrs.size(); ++i) {
		if (attrs.at(i).name == name) return i;
	}
	return -1;
}

QString TcXmlScanner::attribute(int node, const QString &name) const
{
	const int i = attributeIndex(node, name);
	return i < 0 ? QString() : m_nodes.at(node).attributes.at(i).value;
}

/**
 * @brief TcXmlScanner::text
 * @return Decoded text directly inside @p node (child elements skipped).
 */
QString TcXmlScanner::text(int node) const
{
	const Node &nd = m_nodes.at(node);
	QString result;
	int pos = nd.contentBegin;
	int childIdx = 0;
	while (pos < nd.contentEnd) {
		if (childIdx < nd.children.size()
			&& pos >= m_nodes.at(nd.children.at(childIdx)).startBegin) {
			pos = m_nodes.at(nd.children.at(childIdx)).endEnd;
			++childIdx;
			continue;
		}
		const int limit = childIdx < nd.children.size()
			? m_nodes.at(nd.children.at(childIdx)).startBegin
			: nd.contentEnd;
		if (m_text.mid(pos, 9) == QLatin1String("<![CDATA[")) {
			const int end = m_text.indexOf(QLatin1String("]]>"), pos + 9);
			result += m_text.mid(pos + 9, end - pos - 9);
			pos = end + 3;
		} else if (m_text.mid(pos, 4) == QLatin1String("<!--")) {
			const int end = m_text.indexOf(QLatin1String("-->"), pos + 4);
			pos = end + 3;
		} else {
			int next = pos;
			while (next < limit && m_text.at(next) != QLatin1Char('<')) ++next;
			if (next == pos) ++next;
			result += decodeEntities(m_text.mid(pos, next - pos));
			pos = next;
		}
	}
	return result;
}

bool TcXmlScanner::hasCData(int node) const
{
	const Node &nd = m_nodes.at(node);
	const int idx = m_text.indexOf(QLatin1String("<![CDATA["), nd.contentBegin);
	return idx >= 0 && idx < nd.contentEnd;
}

/**
 * @brief TcXmlScanner::structureSignature
 * One line per element (parent index, name, attribute names). Two documents
 * with the same signature have the same structure, whatever the values.
 */
QStringList TcXmlScanner::structureSignature(const QString &ignore) const
{
	QStringList sig;
	QVector<QString> paths(m_nodes.size());
	QVector<bool> skipped(m_nodes.size(), false);
	sig.reserve(m_nodes.size());
	for (int i = 0; i < m_nodes.size(); ++i) {
		const Node &nd = m_nodes.at(i);
		const bool parent_skipped = nd.parent >= 0 && skipped.at(nd.parent);
		skipped[i] = parent_skipped || (!ignore.isEmpty() && nd.name == ignore);
		if (skipped.at(i)) continue;
		paths[i] = (nd.parent >= 0 ? paths.at(nd.parent) : QString())
			+ QLatin1Char('/') + nd.name;
		QStringList names;
		for (const Attribute &a : nd.attributes) names << a.name;
		sig << paths.at(i) + QLatin1Char('@') + names.join(QLatin1Char(','));
	}
	return sig;
}

QString TcXmlScanner::decodeEntities(const QString &raw)
{
	if (!raw.contains(QLatin1Char('&'))) return raw;
	QString out;
	out.reserve(raw.size());
	int i = 0;
	while (i < raw.size()) {
		const QChar c = raw.at(i);
		if (c != QLatin1Char('&')) {
			out += c;
			++i;
			continue;
		}
		const int semi = raw.indexOf(QLatin1Char(';'), i);
		if (semi < 0 || semi - i > 10) {
			out += c;
			++i;
			continue;
		}
		const QString ent = raw.mid(i + 1, semi - i - 1);
		QString rep;
		if (ent == QLatin1String("lt")) rep = QStringLiteral("<");
		else if (ent == QLatin1String("gt")) rep = QStringLiteral(">");
		else if (ent == QLatin1String("amp")) rep = QStringLiteral("&");
		else if (ent == QLatin1String("quot")) rep = QStringLiteral("\"");
		else if (ent == QLatin1String("apos")) rep = QStringLiteral("'");
		else if (ent.startsWith(QLatin1Char('#'))) {
			bool ok = false;
			const uint code = ent.at(1) == QLatin1Char('x')
				? ent.mid(2).toUInt(&ok, 16)
				: ent.mid(1).toUInt(&ok, 10);
			if (ok && code > 0) rep = QString::fromUcs4(&code, 1);
		}
		if (rep.isEmpty()) {
			out += c;
			++i;
		} else {
			out += rep;
			i = semi + 1;
		}
	}
	return out;
}

QString TcXmlScanner::escapeAttribute(const QString &value)
{
	QString out = value;
	out.replace(QLatin1Char('&'), QLatin1String("&amp;"));
	out.replace(QLatin1Char('<'), QLatin1String("&lt;"));
	out.replace(QLatin1Char('"'), QLatin1String("&quot;"));
	return out;
}

QString TcXmlScanner::escapeText(const QString &value)
{
	QString out = value;
	out.replace(QLatin1Char('&'), QLatin1String("&amp;"));
	out.replace(QLatin1Char('<'), QLatin1String("&lt;"));
	out.replace(QLatin1Char('>'), QLatin1String("&gt;"));
	return out;
}
