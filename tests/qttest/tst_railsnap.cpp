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
	//Settings > General: a symbol which declares itself a component of a
	//rail (<railSnap> in its definition) is put on the axis of the nearest
	//rail and against its neighbour, 2 mm away by default
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagramevent/diagrameventaddelement.h"
#include "factory/elementfactory.h"
#include "properties/railsnapdata.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "railsnap.h"

#include <QGraphicsSceneMouseEvent>
#include <QUndoStack>

class tst_railsnap : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

			//A symbol whose body is a rectangle starting at its origin
		void define(const QString &name, const QString &railsnap)
		{
			QDomDocument definition;
			const QString text = QStringLiteral(
				"<definition type=\"element\" link_type=\"simple\" "
				"width=\"500\" height=\"100\" hotspot_x=\"10\" hotspot_y=\"10\">"
				"<names><name lang=\"en\">%1</name></names>%2"
				"<description><rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" "
				"style=\"line-style:normal;line-weight:normal;"
				"filling:none;color:black\"/></description></definition>")
				.arg(name, railsnap);
			QVERIFY(definition.setContent(text));
			QVERIFY(m_diagram->project()->embeddedElementCollection()
					->addElementDefinition(QStringLiteral("import"), name,
										   definition.documentElement()));
		}

		static QString snap(const QString &role, const QString &axis,
							const QString &size, const QString &more)
		{
			return QStringLiteral("<railSnap role=\"%1\" axis=\"%2\" x=\"0\" "
								  "y=\"0\" %3 %4/>")
				.arg(role, axis, size, more);
		}

		Element *place(const QString &name, const QPointF &pos,
					   bool selected = false)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/%1.elmt")
								 .arg(name), m_diagram->project()),
				nullptr, &state);
			if (!element || state) {
				return nullptr;
			}
			m_diagram->addItem(element);
			element->QGraphicsItem::setPos(pos);
			element->setSelected(selected);
			return element;
		}

		void send(QEvent::Type type, const QPointF &pos, const QPointF &down,
				  Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
		{
			QGraphicsSceneMouseEvent event(type);
			event.setScenePos(pos);
			event.setButtonDownScenePos(Qt::LeftButton, down);
			event.setButton(type == QEvent::GraphicsSceneMouseMove
							? Qt::NoButton : Qt::LeftButton);
			event.setButtons(buttons);
			event.setModifiers(modifiers);
			QApplication::sendEvent(m_diagram, &event);
		}

			//Drag by @a by from the inside of @a element
		void drag(Element *element, const QPointF &by,
				  Qt::KeyboardModifiers modifiers = Qt::NoModifier)
		{
			const QPointF down = element->pos() + QPointF(10, 10);
			send(QEvent::GraphicsSceneMousePress, down, down,
				 Qt::LeftButton, modifiers);
			send(QEvent::GraphicsSceneMouseMove, down + by / 2, down,
				 Qt::LeftButton, modifiers);
			send(QEvent::GraphicsSceneMouseMove, down + by, down,
				 Qt::LeftButton, modifiers);
			send(QEvent::GraphicsSceneMouseRelease, down + by, down,
				 Qt::NoButton, modifiers);
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			Diagram::setGridSnap(false);
				//1 px = 2 mm: a rail 20 px (40 mm) across, with its axis in
				//the middle, and devices 40 mm high fixed by their middle
			const QString size{"width=\"400\" height=\"20\""};
			define("rail", snap("rail", "horizontal", size,
								"mount=\"center\" mm_per_px=\"2\""));
			define("vrail", snap("rail", "vertical",
								 "width=\"20\" height=\"400\"",
								 "mount=\"center\" mm_per_px=\"2\""));
			const QString body{"width=\"20\" height=\"40\""};
			define("dev2", snap("component", "horizontal", body,
								"mount=\"center\" mm_per_px=\"2\""));
			define("dev1", snap("component", "horizontal", body,
								"mount=\"center\" mm_per_px=\"1\""));
			define("dev5", snap("component", "horizontal", body,
								"mount=\"center\" mm_per_px=\"5\""));
			define("gap4", snap("component", "horizontal", body,
								"mount=\"center\" mm_per_px=\"2\" "
								"gap_mm=\"6\""));
			define("start", snap("component", "horizontal", body,
								 "mount=\"start\" mm_per_px=\"2\""));
			define("end", snap("component", "horizontal", body,
							   "mount=\"end\" mm_per_px=\"2\""));
			define("origin", snap("component", "horizontal", body,
								  "mount=\"origin\" mm_per_px=\"2\""));
			define("offset", snap("component", "horizontal", body,
								  "mount=\"center\" mount_offset_mm=\"4\" "
								  "mm_per_px=\"2\""));
			define("vdev", snap("component", "vertical",
								"width=\"40\" height=\"20\"",
								"mount=\"center\" mm_per_px=\"2\""));
			define("plain", QString());
		}

		void init()
		{
			for (QGraphicsItem *item : m_diagram->items())
				if (item->type() == Element::Type)
					delete item;
			RailSnap::setEnabled(true);
			RailSnap::setThresholdMm(10);
			m_diagram->undoStack().clear();
		}

		void cleanupTestCase()
		{
			RailSnap::setEnabled(false);
		}

			//Off by default, so that nothing changes for existing projects
		void offByDefault()
		{
			QSettings().remove(QStringLiteral("diagrameditor/rail_snap"));
			QSettings().remove(
				QStringLiteral("diagrameditor/rail_snap_threshold_mm"));
			QCOMPARE(RailSnap::thresholdMm(), 10.0);
			QVERIFY(!QSettings().value(
				QStringLiteral("diagrameditor/rail_snap"), false).toBool());
		}

		void dataIsRead()
		{
			Element *dev = place("dev2", QPointF(0, 0));
			QVERIFY(dev);
			const RailSnapData &data = dev->railSnapData();
			QVERIFY(data.isComponent());
			QCOMPARE(data.m_body, QRectF(0, 0, 20, 40));
			QCOMPARE(data.m_gap_mm, 2.0);
			QCOMPARE(data.m_mm_per_px, 2.0);
			QCOMPARE(data.fixCoordinate(), 20.0);
			QVERIFY(data.isAssumed());
			QVERIFY(place("rail", QPointF(0, 0))->railSnapData().isRail());
			QVERIFY(!place("plain", QPointF(0, 0))->railSnapData().isValid());
		}

			//The fixing line of the device goes onto the axis of the rail
		void deviceIsPutOnTheRail()
		{
			place("rail", QPointF(50, 100));
			Element *dev = place("dev2", QPointF(100, 93));
			const auto magnet = dev->magneticPosition(QPointF(100, 93),
													  Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(*magnet, QPointF(100, 90));
			QVERIFY(dev->magneticPosition(QPointF(100, 87),
										  Qt::NoModifier)
					== QPointF(100, 90));
		}

			//Devices of different heights share the line of the rail
		void fixingModesPutTheirLineOnTheAxis()
		{
			place("rail", QPointF(50, 100));
			const struct {const char *m_name; qreal m_from; qreal m_y;}
			modes[] = {
				{"dev2", 93, 90}, {"start", 113, 110}, {"end", 73, 70},
				{"origin", 113, 110}, {"offset", 91, 88}};
			for (const auto &mode : modes) {
				Element *dev = place(QString::fromLatin1(mode.m_name),
									 QPointF(100, mode.m_from));
				const auto magnet = dev->magneticPosition(
					QPointF(100, mode.m_from), Qt::NoModifier);
				QVERIFY2(magnet, mode.m_name);
				QCOMPARE(*magnet, QPointF(100, mode.m_y));
			}
		}

			//The nearest rail wins
		void nearestRailIsChosen()
		{
			place("rail", QPointF(50, 100));
			place("rail", QPointF(50, 106));
			Element *dev = place("dev2", QPointF(100, 92));
			const auto magnet = dev->magneticPosition(QPointF(100, 92),
													  Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(magnet->y(), 90.0);
		}

			//2 mm is 1 px at 1 px = 2 mm, 2 px at 1 px = 1 mm
		void neighbourIsAt2mmAtAnyScale_data()
		{
			QTest::addColumn<QString>("name");
			QTest::addColumn<qreal>("gap");
			QTest::newRow("2 mm per px") << "dev2" << 1.0;
			QTest::newRow("1 mm per px") << "dev1" << 2.0;
			QTest::newRow("5 mm per px") << "dev5" << 0.4;
		}

		void neighbourIsAt2mmAtAnyScale()
		{
			QFETCH(QString, name);
			QFETCH(qreal, gap);
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
				//Its right edge is at 120: to the right, then to the left
			Element *dev = place(name, QPointF(122, 91));
			auto magnet = dev->magneticPosition(QPointF(122, 91),
												Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(*magnet, QPointF(120 + gap, 90));
			magnet = dev->magneticPosition(QPointF(78, 91), Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(*magnet, QPointF(100 - gap - 20, 90));
		}

		void gapIsTheOneOfTheDevice()
		{
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
			Element *dev = place("gap4", QPointF(124, 90));
			const auto magnet = dev->magneticPosition(QPointF(124, 90),
													  Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(magnet->x(), 123.0);
		}

			//Too far from the neighbour: only the height changes
		void neighbourOutOfReachIsIgnored()
		{
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
			Element *dev = place("dev2", QPointF(140, 93));
			const auto magnet = dev->magneticPosition(QPointF(140, 93),
													  Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(*magnet, QPointF(140, 90));
		}

			//Never on top of a device which is already there
		void deviceBetweenTwoNeighbours()
		{
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
			place("dev2", QPointF(150, 90));
			Element *dev = place("dev2", QPointF(125, 93));
			const auto magnet = dev->magneticPosition(QPointF(125, 93),
													  Qt::NoModifier);
			QVERIFY(magnet);
				//Against the first: 121 to 141, which is free; the
				//other candidate, 129, is the nearest and also free
			QCOMPARE(magnet->y(), 90.0);
			QVERIFY(magnet->x() >= 121 && magnet->x() <= 129);
		}

		void farFromTheRailNothingMoves()
		{
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
			Element *dev = place("dev2", QPointF(122, 150));
			QVERIFY(!dev->magneticPosition(QPointF(122, 150),
										   Qt::NoModifier));
				//Beside the rail: the rail is not within reach along it
			QVERIFY(!place("dev2", QPointF(600, 93))->magneticPosition(
				QPointF(600, 93), Qt::NoModifier));
		}

		void reachFollowsTheSetting()
		{
			place("rail", QPointF(50, 100));
			Element *dev = place("dev2", QPointF(100, 85));
				//Fixing line 5 px = 10 mm from the axis: in reach at 10 mm
			QVERIFY(dev->magneticPosition(QPointF(100, 85), Qt::NoModifier));
			RailSnap::setThresholdMm(4);
			QVERIFY(!dev->magneticPosition(QPointF(100, 85), Qt::NoModifier));
			QVERIFY(dev->magneticPosition(QPointF(100, 89), Qt::NoModifier));
		}

		void switchedOffByTheSettingAndTheKeys()
		{
			place("rail", QPointF(50, 100));
			Element *dev = place("dev2", QPointF(100, 93));
			QVERIFY(dev->magneticPosition(QPointF(100, 93), Qt::NoModifier));
			QVERIFY(!dev->magneticPosition(QPointF(100, 93),
										   Qt::AltModifier));
			QVERIFY(!dev->magneticPosition(QPointF(100, 93),
										   Qt::ShiftModifier));
			QVERIFY(dev->magneticPosition(QPointF(100, 93),
										  Qt::ControlModifier));
			RailSnap::setEnabled(false);
			QVERIFY(!dev->magneticPosition(QPointF(100, 93),
										   Qt::NoModifier));
		}

		void symbolsWithoutDataAreNotTouched()
		{
			place("rail", QPointF(50, 100));
			Element *plain = place("plain", QPointF(100, 93));
			QVERIFY(!plain->magneticPosition(QPointF(100, 93),
											 Qt::NoModifier));
			QVERIFY(!place("rail", QPointF(0, 0))->magneticPosition(
				QPointF(50, 98), Qt::NoModifier));
		}

		void verticalRailGivesTheSameRuleAcross()
		{
			place("vrail", QPointF(100, 50));
			place("vdev", QPointF(90, 100));
			Element *dev = place("vdev", QPointF(93, 122));
			const auto magnet = dev->magneticPosition(QPointF(93, 122),
													  Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(*magnet, QPointF(90, 120 + 1));
				//A horizontal rail does not attract a vertical device
			place("rail", QPointF(300, 100));
			QVERIFY(!place("vdev", QPointF(300, 93))->magneticPosition(
				QPointF(300, 93), Qt::NoModifier));
		}

		void turnedSymbolHasNoRailDirection()
		{
			place("rail", QPointF(50, 100));
			Element *dev = place("dev2", QPointF(100, 93));
			dev->QGraphicsItem::setRotation(45);
			QVERIFY(!dev->magneticPosition(QPointF(100, 93),
										   Qt::NoModifier));
			dev->QGraphicsItem::setRotation(180);
				//Turned half a turn, the fixing line is 20 px above
			dev->QGraphicsItem::setPos(QPointF(100, 133));
			const auto magnet = dev->magneticPosition(QPointF(100, 133),
													  Qt::NoModifier);
			QVERIFY(magnet);
			QCOMPARE(*magnet, QPointF(100, 130));
		}

			//One move of the mouse, one step of undo
		void dragSnapsAndUndoesInOneStep()
		{
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
			Element *dev = place("dev2", QPointF(200, 93), true);
			const QPointF start = dev->pos();
			drag(dev, QPointF(-77, 0));
			QCOMPARE(dev->pos(), QPointF(121, 90));
			QCOMPARE(m_diagram->undoStack().count(), 1);
			m_diagram->undoStack().undo();
			QCOMPARE(dev->pos(), start);
		}

		void dragWithAltKeyDoesNotSnap()
		{
			place("rail", QPointF(50, 100));
			Element *dev = place("dev2", QPointF(200, 93), true);
			drag(dev, QPointF(-77, 0), Qt::AltModifier);
			QCOMPARE(dev->pos().y(), 93.0);
		}

			//The others of the selection keep their place relative to
			//the one the mouse holds
		void groupKeepsItsRelativePositions()
		{
			place("rail", QPointF(50, 100));
			Element *a = place("dev2", QPointF(100, 93), true);
			Element *b = place("dev2", QPointF(160, 93), true);
			drag(a, QPointF(4, 0));
			QCOMPARE(a->pos(), QPointF(104, 90));
			QCOMPARE(b->pos() - a->pos(), QPointF(60, 0));
			QCOMPARE(m_diagram->undoStack().count(), 1);
			m_diagram->undoStack().undo();
			QCOMPARE(a->pos(), QPointF(100, 93));
			QCOMPARE(b->pos(), QPointF(160, 93));
		}

			//Putting a symbol down: the tool snaps and places it there,
			//and one undo takes it away
		void placingSnapsAndUndoes()
		{
			place("rail", QPointF(50, 100));
			place("dev2", QPointF(100, 90));
			const int count = m_diagram->elements().count();
			ElementsLocation location(
				QStringLiteral("embed://import/dev2.elmt"),
				m_diagram->project());
			DiagramEventAddElement tool(location, m_diagram);
			QVERIFY(tool.isRunning());
			const QPointF at(124, 93);
			for (QEvent::Type type : {QEvent::GraphicsSceneMouseMove,
									  QEvent::GraphicsSceneMousePress,
									  QEvent::GraphicsSceneMouseRelease}) {
				QGraphicsSceneMouseEvent event(type);
				event.setButton(Qt::LeftButton);
				event.setButtons(type == QEvent::GraphicsSceneMousePress
								 ? Qt::LeftButton : Qt::NoButton);
				event.setScenePos(at);
				if (type == QEvent::GraphicsSceneMouseMove)
					tool.mouseMoveEvent(&event);
				else if (type == QEvent::GraphicsSceneMousePress)
					tool.mousePressEvent(&event);
				else
					tool.mouseReleaseEvent(&event);
			}
			QCOMPARE(m_diagram->undoStack().count(), 1);
			QGraphicsSceneMouseEvent finish(
				QEvent::GraphicsSceneMouseRelease);
			finish.setButton(Qt::RightButton);
			tool.mouseReleaseEvent(&finish);
			QCOMPARE(m_diagram->elements().count(), count + 1);
			bool found{false};
			for (const Element *e : m_diagram->elements())
				found |= e->pos() == QPointF(121, 90);
			QVERIFY(found);
			m_diagram->undoStack().undo();
			QCOMPARE(m_diagram->elements().count(), count);
		}
};

QET_APP_TEST_MAIN(tst_railsnap)
#include "tst_railsnap.moc"
