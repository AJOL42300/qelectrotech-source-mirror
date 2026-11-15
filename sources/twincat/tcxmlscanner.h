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
#ifndef TCXMLSCANNER_H
#define TCXMLSCANNER_H

#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief The TcXmlScanner class
 * Minimal XML scanner that remembers where every element, attribute value
 * and text lives in the original text. TwinCAT files are patched in place
 * from these offsets, so that everything outside the edited values stays
 * identical to the original, byte for byte.
 * Comments, processing instructions and DOCTYPE are skipped.
 */
class TcXmlScanner
{
	public:
		struct Attribute {
			QString name;
			QString value;      ///< decoded value
			int rawBegin = 0;   ///< first character of the value (after the quote)
			int rawEnd = 0;     ///< position of the closing quote
		};

		struct Node {
			QString name;
			QVector<Attribute> attributes;
			int parent = -1;
			QVector<int> children;
			int startBegin = 0;   ///< position of '<'
			int startEnd = 0;     ///< position after '>' of the start tag
			int contentBegin = 0; ///< same as startEnd
			int contentEnd = 0;   ///< position of "</" (or startEnd if empty)
			int endEnd = 0;       ///< position after the end tag
			bool selfClosing = false;
		};

		bool parse(const QString &text);
		QString errorString() const { return m_error; }

		int root() const { return m_root; }
		const QVector<Node> &nodes() const { return m_nodes; }
		const Node &node(int index) const { return m_nodes.at(index); }
		int count() const { return m_nodes.size(); }

		int child(int node, const QString &name) const;
		QVector<int> childrenNamed(int node, const QString &name) const;
		int attributeIndex(int node, const QString &name) const;
		QString attribute(int node, const QString &name) const;
		QString text(int node) const;
		bool hasCData(int node) const;
		QStringList structureSignature(const QString &ignore = QString()) const;

		static QString decodeEntities(const QString &raw);
		static QString escapeAttribute(const QString &value);
		static QString escapeText(const QString &value);

	private:
		QString m_text;
		QString m_error;
		QVector<Node> m_nodes;
		int m_root = -1;
};

#endif // TCXMLSCANNER_H
