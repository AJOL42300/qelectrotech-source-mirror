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
#include "functionpropagation.h"

#include "ElementsCollection/elementslocation.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "undocommand/changeelementdatacommand.h"
#include "undocommand/changeelementinformationcommand.h"

#include <QSet>
#include <QSettings>
#include <QUndoCommand>

namespace
{
	const QString function_key = QStringLiteral("function");
	const QString routes_name = QStringLiteral("function_routes");
	const QString ends_name = QStringLiteral("function_ends");

		/// A child command being built must not follow in its turn
	bool building = false;

	struct Port
	{
		QString terminal;
		int channel = 0;

		bool isValid() const { return !terminal.isEmpty() && channel > 0; }
		bool operator==(const Port &other) const {
			return terminal == other.terminal && channel == other.channel;
		}
	};

	struct Route
	{
		Port from;
		Port to;
	};

	struct End
	{
		Element *element = nullptr;
		Port port;
	};

	Port parsePort(const QString &text)
	{
		Port port;
		const int colon = text.lastIndexOf(QLatin1Char(':'));
		if (colon <= 0) {
			return port;
		}
		bool ok = false;
		const int channel = text.mid(colon + 1).trimmed().toInt(&ok);
		if (ok) {
			port.terminal = text.left(colon).trimmed();
			port.channel = channel;
		}
		return port;
	}

	QString kindInformation(const ElementData &data, const QString &name)
	{
		for (const auto &pair : data.m_other_kind_informations) {
			if (pair.first == name) {
				return pair.second;
			}
		}
		return QString();
	}

	QList<Route> routesOf(const ElementData &data)
	{
		QList<Route> routes;
		const auto items = kindInformation(data, routes_name)
				.split(QLatin1Char(';'), Qt::SkipEmptyParts);
		for (const auto &item : items) {
			const int arrow = item.indexOf(QLatin1Char('>'));
			if (arrow <= 0) {
				continue;
			}
			Route route{parsePort(item.left(arrow)),
				    parsePort(item.mid(arrow + 1))};
			if (route.from.isValid() && route.to.isValid()) {
				routes << route;
			}
		}
		return routes;
	}

	QList<Port> endsOf(const ElementData &data)
	{
		QList<Port> ends;
		const auto items = kindInformation(data, ends_name)
				.split(QLatin1Char(';'), Qt::SkipEmptyParts);
		for (const auto &item : items) {
			const Port port = parsePort(item);
			if (port.isValid()) {
				ends << port;
			}
		}
		return ends;
	}

	bool isPlc(const ElementData &data)
	{
		return data.m_type == ElementData::Master
			&& data.m_master_type == ElementData::PLC;
	}

		/// Same rule as the tooltip of a module: a channel that names its
		/// terminals belongs to them, else its address begins with the pin
	bool ioBelongsTo(const ElementData::PlcIO &io, const QString &pin)
	{
		if (!io.terminals.isEmpty()) {
			return io.terminals.contains(pin);
		}
		return io.address == pin
			|| io.address.startsWith(pin + QLatin1Char('('));
	}

	QStringList pinsOf(const ElementData::PlcIO &io)
	{
		if (!io.terminals.isEmpty()) {
			return io.terminals;
		}
		const int paren = io.address.indexOf(QLatin1Char('('));
		return QStringList(paren > 0 ? io.address.left(paren)
					     : io.address);
	}

		/// Index in the table of the channel of a terminal, or -1
	int ioIndex(const ElementData &data, const Port &port)
	{
		int channel = 0;
		const auto &ios = data.m_plc_master_data.ios;
		for (int i = 0; i < ios.size(); ++i) {
			if (ioBelongsTo(ios.at(i), port.terminal)
			    && ++channel == port.channel) {
				return i;
			}
		}
		return -1;
	}

	bool isBound(const ElementData &data, const Port &port)
	{
		if (isPlc(data)) {
			return ioIndex(data, port) >= 0;
		}
		return endsOf(data).contains(port);
	}

	QString functionOf(const End &end)
	{
		const ElementData data = end.element->elementData();
		if (isPlc(data)) {
			const int index = ioIndex(data, end.port);
			return index < 0 ? QString()
				: data.m_plc_master_data.ios.at(index)
					.functionText;
		}
		return end.element->elementInformations()
				.value(function_key).toString();
	}

		/// The function of a channel as the user set it: the text a PLC
		/// symbol brings from its definition ("In-/Output A") is not one
	QString setFunctionOf(const End &end)
	{
		const QString value = functionOf(end);
		const ElementData data = end.element->elementData();
		if (value.isEmpty() || !isPlc(data)) {
			return value;
		}
		const ElementsLocation location = end.element->location();
		ElementData original;
		if (location.isElement() && original.fromXml(location.xml())) {
			const int index = ioIndex(original, end.port);
			if (index >= 0 && original.m_plc_master_data.ios.at(index)
					.functionText == value) {
				return QString();
			}
		}
		return value;
	}

	Terminal *terminalNamed(const Element *element, const QString &name)
	{
		for (Terminal *terminal : element->terminals()) {
			if (terminal->name() == name) {
				return terminal;
			}
		}
		return nullptr;
	}

		/// Channels reached from a terminal through the cables
		/// (conductors) and the symbols that pass the channels through
	void walk(const Terminal *terminal,
		  int channel,
		  bool crossed,
		  QSet<QPair<const Terminal *, int>> &visited,
		  QList<End> &ends)
	{
		for (const Conductor *conductor : terminal->conductors()) {
			Terminal *other = conductor->terminal1 == terminal
				? conductor->terminal2
				: conductor->terminal1;
			Element *element = other
				? other->parentElement() : nullptr;
			if (!element) {
				continue;
			}
			const Port here{other->name(), channel};
			if (visited.contains({other, channel})) {
				continue;
			}
			visited.insert({other, channel});

			const ElementData data = element->elementData();
			if (crossed && isBound(data, here)) {
				ends << End{element, here};
			}
			for (const Route &route : routesOf(data)) {
				Port next;
				if (route.from == here) {
					next = route.to;
				} else if (route.to == here) {
					next = route.from;
				} else {
					continue;
				}
				Terminal *next_terminal =
					terminalNamed(element, next.terminal);
				if (!next_terminal) {
					continue;
				}
				visited.insert({next_terminal, next.channel});
				walk(next_terminal, next.channel, true,
				     visited, ends);
			}
		}
	}

	QList<End> reachableEnds(const End &source)
	{
		QList<End> ends;
		Terminal *terminal =
			terminalNamed(source.element, source.port.terminal);
		if (!terminal) {
			return ends;
		}
		QSet<QPair<const Terminal *, int>> visited;
		visited.insert({terminal, source.port.channel});
		walk(terminal, source.port.channel, false, visited, ends);
		return ends;
	}

	QString nameOf(const Element *element)
	{
		const QString label = element->elementInformations()
				.value(QStringLiteral("label")).toString();
		return label.isEmpty() ? element->name() : label;
	}

	struct Followers
	{
		QMap<Element *, ElementData> data;
		QMap<Element *, DiagramContext> infos;
	};

	void setFunction(const End &end, const QString &value, Followers &out)
	{
		const ElementData current = end.element->elementData();
		if (isPlc(current)) {
			ElementData data = out.data.value(
				end.element, current);
			const int index = ioIndex(data, end.port);
			if (index >= 0) {
				data.m_plc_master_data.ios[index]
					.functionText = value;
				out.data.insert(end.element, data);
			}
			return;
		}
		DiagramContext context = out.infos.value(
			end.element, end.element->elementInformations());
		context.addValue(function_key, value);
		out.infos.insert(end.element, context);
	}

		/// The function of a channel changed from old_value to new_value:
		/// give it to the other ends. An end that is empty or still holds
		/// the old value follows, one that holds the new value is done,
		/// any other function is kept and reported.
	void propagate(const End &source,
		       const QString &old_value,
		       const QString &new_value,
		       const QSet<Element *> &skipped,
		       Followers &out)
	{
		for (const End &end : reachableEnds(source)) {
			if (end.element == source.element
			    || skipped.contains(end.element)) {
				continue;
			}
			const QString current = setFunctionOf(end);
			if (current == new_value) {
				continue;
			}
			if (current.isEmpty() || current == old_value) {
				setFunction(end, new_value, out);
				continue;
			}
			emit FunctionPropagation::notifier()->conflictFound(
				FunctionPropagation::tr(
					"The function \"%1\" of %2 is not "
					"given to %3, which has the function "
					"\"%4\". Both are kept.")
				.arg(new_value,
				     nameOf(source.element),
				     nameOf(end.element),
				     current));
		}
	}

	void addChildren(QUndoCommand *parent, const Followers &out)
	{
		if (out.data.isEmpty() && out.infos.isEmpty()) {
			return;
		}
		building = true;
		for (auto it = out.data.begin(); it != out.data.end(); ++it) {
			new ChangeElementDataCommand(it.key(), it.value(),
						     parent);
		}
		for (auto it = out.infos.begin();
		     it != out.infos.end(); ++it) {
			new ChangeElementInformationCommand(
				it.key(),
				it.key()->elementInformations(),
				it.value(),
				parent);
		}
		building = false;
	}
}

QString FunctionPropagation::settingsKey()
{
	return QStringLiteral("project/enable-function-propagation");
}

bool FunctionPropagation::isEnabled()
{
	return QSettings().value(settingsKey(), false).toBool();
}

FunctionPropagation *FunctionPropagation::notifier()
{
	static FunctionPropagation instance;
	return &instance;
}

/**
	@brief FunctionPropagation::follow
	Adds, as children of parent, the commands that give the new function
	of the channels of the edited symbols to the symbols at the other end.
	@param parent : command that edits the information of the symbols
	@param infos : for each edited symbol, its old and new information
*/
void FunctionPropagation::follow(
		QUndoCommand *parent,
		const QMap<QPointer<Element>,
		           QPair<DiagramContext, DiagramContext>> &infos)
{
	if (building || !parent || !isEnabled()) {
		return;
	}
	QSet<Element *> edited;
	for (auto it = infos.begin(); it != infos.end(); ++it) {
		if (it.key()) {
			edited.insert(it.key().data());
		}
	}

	Followers out;
	for (auto it = infos.begin(); it != infos.end(); ++it) {
		Element *element = it.key().data();
		if (!element || !element->diagram()) {
			continue;
		}
		const QString old_value =
			it.value().first.value(function_key).toString();
		const QString new_value =
			it.value().second.value(function_key).toString();
		if (old_value == new_value) {
			continue;
		}
		for (const Port &port : endsOf(element->elementData())) {
			propagate(End{element, port}, old_value, new_value,
				  edited, out);
		}
	}
	addChildren(parent, out);
}

/**
	@brief FunctionPropagation::follow
	Same as above for the edition of the data of a symbol, as the table
	of the channels of a module.
	@param parent : command that edits the data of the symbol
	@param element : edited symbol
	@param new_data : data of the symbol after the edition
*/
void FunctionPropagation::follow(
		QUndoCommand *parent,
		Element *element,
		const ElementData &new_data)
{
	if (building || !parent || !element || !element->diagram()
	    || !isEnabled()) {
		return;
	}
	const ElementData old_data = element->elementData();
	if (!isPlc(old_data) || !isPlc(new_data)) {
		return;
	}
	const auto &old_ios = old_data.m_plc_master_data.ios;
	const auto &new_ios = new_data.m_plc_master_data.ios;
	if (old_ios.size() != new_ios.size()) {
		return;
	}

	Followers out;
	for (int i = 0; i < old_ios.size(); ++i) {
		const QString old_value = old_ios.at(i).functionText;
		const QString new_value = new_ios.at(i).functionText;
		if (old_value == new_value) {
			continue;
		}
		for (const QString &pin : pinsOf(old_ios.at(i))) {
			int channel = 0;
			for (int j = 0; j <= i; ++j) {
				if (ioBelongsTo(old_ios.at(j), pin)) {
					++channel;
				}
			}
			propagate(End{element, Port{pin, channel}},
				  old_value, new_value,
				  QSet<Element *>{element}, out);
		}
	}
	addChildren(parent, out);
}
