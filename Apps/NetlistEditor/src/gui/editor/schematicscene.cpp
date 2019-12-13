/*
    QSapecNG - Qt based SapecNG GUI front-end
    Copyright (C) 2009, Michele Caini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "parser/parser_factory.h"

#include "gui/editor/schematicscene.h"
#include "gui/editor/schematicsceneparser.h"
#include "gui/editor/undoredocommand.h"
#include "gui/editor/component.h"
#include "gui/editor/label.h"
#include "gui/editor/wire.h"
#include "gui/editor/item.h"

#include "gui/settings.h"
#include "gui/qlogger.h"

#include "QtBoolPropertyManager"
#include "QtGroupPropertyManager"

#include "QtLineEditFactory"
#include "QtCheckBoxFactory"

#include "QtAbstractPropertyBrowser"

#include <QMessageBox>

#include <QtCore/QEvent>
#include <QtCore/QRegExp>
#include <QtCore/QCryptographicHash>
#include <QtCore/QPointer>

#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QFileDialog>
#include <QUndoCommand>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QtVariantProperty>

#include <sstream>
#include <memory>

#include "utility/qsapecngUtility.h"
#include "utility/strings.h"


namespace qsapecng
{


QPainterPath SchematicScene::nilPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, 0);
  path.lineTo(0, step);
  path.lineTo(step, step);
  path.lineTo(step, 0);
  path.lineTo(0, 0);

  return path;
}


QPainterPath SchematicScene::groundPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(step, 0);
  path.lineTo(step, step);
  path.moveTo(step - step / 2, step);
  path.lineTo(step + step / 2, step);
  path.moveTo(step - step / 4, step + step / 3);
  path.lineTo(step + step / 4, step + step / 3);
  path.moveTo(step - step / 6, step + step * 2/3);
  path.lineTo(step + step / 6, step + step * 2/3);
  path.addRect(step, 2 * step, 1E-99, 1E-99);

  return path;
}


QPainterPath SchematicScene::portPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.addRect(0, 0, 1E-99, 1E-99);
  path.addRect(2 * step, 2 * step, 1E-99, 1E-99);
  path.moveTo(step / 3, step);
  path.arcTo(
      QRectF(step / 3, step / 3, 2 * step - step * 2 / 3, 2 * step - step * 2 / 3),
      180, 360
    );
  path.moveTo(step / 2, step);
  path.arcTo(
      QRectF(step / 2, step / 2, step, step),
      180, 360
    );

  return path;
}


QPainterPath SchematicScene::outPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, step);
  path.lineTo(step, step);
  path.arcTo(
      QRectF(step, step / 2, step, step),
      180, 360
    );

  return path;
}


QPainterPath SchematicScene::voltmeterPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(step, 0);
  path.lineTo(step, step + step / 2);
  path.moveTo(step, 3 * step + step / 2);
  path.arcTo(
    QRectF(
      QPointF(0, step + step / 2),
      QPointF(2 * step, 3 * step + step / 2)
    ), -90, 360
  );
  path.lineTo(step, 5 * step);

  path.moveTo(step - step / 2, 2 * step);
  path.lineTo(step, 3 * step);
  path.lineTo(step + step / 2, 2 * step);

  path.moveTo(step + step / 2, step / 2 - step / 4);
  path.lineTo(step + step / 2, step / 2 + step / 4);

  path.moveTo(step + step / 2 - step / 4, step / 2);
  path.lineTo(step + step / 2 + step / 4, step / 2);

  path.moveTo(step + step / 2 - step / 4, 4 * step + step / 2);
  path.lineTo(step + step / 2 + step / 4, 4 * step + step / 2);

  return path;
}


QPainterPath SchematicScene::ammeterPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(step, 0);
  path.lineTo(step, step + step / 2);
  path.moveTo(step, 3 * step + step / 2);
  path.arcTo(
    QRectF(
      QPointF(0, step + step / 2),
      QPointF(2 * step, 3 * step + step / 2)
    ), -90, 360
  );
  path.lineTo(step, 5 * step);

  path.moveTo(step + step / 2, 3 * step);
  path.lineTo(step, 2 * step);
  path.lineTo(step - step / 2, 3 * step);

  path.moveTo(step + step / 4, 2 * step + step / 2);
  path.lineTo(step - step / 4, 2 * step + step / 2);

  path.moveTo(step / 2, 5 * step / 4);
  path.lineTo(step / 2, step / 4);
  path.moveTo(step / 2 - step / 4, 2 * step / 4);
  path.lineTo(step / 2, step / 4);
  path.moveTo(step / 2 + step / 4, 2 * step / 4);
  path.lineTo(step / 2, step / 4);

  return path;
}


QPainterPath SchematicScene::capacitorPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, step);
  path.lineTo(2 * step, step);
  path.moveTo(2 * step, 0);
  path.lineTo(2 * step, 2 * step);
  path.moveTo(3 * step, 0);
  path.lineTo(3 * step, 2 * step);
  path.moveTo(3 * step, step);
  path.lineTo(5 * step, step);

  return path;
}

QPainterPath SchematicScene::volumeTrackingPressureChamberPath()
{
	QPainterPath path;
	uint step = GridStep;

	// This has an arrow which points at the node
	// which should be connected to the rest of the
	// circuit goes (the startNode, at which
	// pressures generated due to the volume (via a control script)
	// will be prescribed.

	path.moveTo(0, step);
	path.lineTo(2 * step, step);
	path.moveTo(2 * step, 0);
	path.lineTo(2 * step, 2 * step);
	path.moveTo(3 * step, 0);
	path.lineTo(3 * step, 2 * step);
	path.moveTo(3 * step, step);
	path.lineTo(5 * step, step);

	path.moveTo(2 * step, 0);
	path.lineTo(3 * step, 0);
	path.moveTo(2 * step, 2 * step);
	path.lineTo(3 * step, 2 * step);

	// Do the arrow which identifies the start-node (see comment above!)
	path.moveTo(4*step - step/2.0, step - step/2.0);
	path.lineTo(4*step, step);
	path.lineTo(4*step - step / 2.0, step + step / 2.0);


	return path;
}

QPainterPath SchematicScene::diodePath()
{
	QPainterPath path;
	uint step = GridStep;

	double cos30 = 0.866;
	double triangleWidth = 2 * step * cos30;
	double sideWireLength = (5 * step - triangleWidth) / 2.0;

	path.moveTo(5 * step, step);
	path.lineTo(5 * step - sideWireLength, step);
	path.moveTo(5 * step - sideWireLength, 2.0 * step);
	path.lineTo(5 * step - sideWireLength, 0.0);
	path.lineTo(5 * step - sideWireLength - triangleWidth, step);
	path.lineTo(5 * step - sideWireLength, 2.0 * step);
	path.moveTo(5 * step - sideWireLength - triangleWidth, step);
	path.lineTo(0, step);
	path.moveTo(5 * step - sideWireLength - triangleWidth, 2.0*step);
	path.lineTo(5 * step - sideWireLength - triangleWidth, 0.0);

	return path;
}


QPainterPath SchematicScene::cccsPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, 0);
  path.lineTo(2 * step, 0);
  path.lineTo(2 * step, step + step / 2);

  path.lineTo(step, 2 * step + step / 2);
  path.lineTo(2 * step, 3 * step + step / 2);
  path.lineTo(3 * step, 2 * step + step / 2);
  path.lineTo(2 * step, step + step / 2);

  path.moveTo(2 * step, 3 * step + step / 2);
  path.lineTo(2 * step, 5 * step);
  path.lineTo(0, 5 * step);

  path.moveTo(2 * step, 3 * step + step / 4);
  path.lineTo(2 * step, 2 * step - step / 4);

  path.moveTo(2 * step - step / 4, 2 * step + step / 4);
  path.lineTo(2 * step, 2 * step - step / 4);

  path.moveTo(2 * step + step / 4, 2 * step + step / 4);
  path.lineTo(2 * step, 2 * step - step / 4);

  path.moveTo(5 * step, 0);
  path.lineTo(4 * step, 0);
  path.lineTo(4 * step, 5 * step);
  path.lineTo(5 * step, 5 * step);

  path.moveTo(4 * step - step / 3, 2 * step + step / 4);
  path.lineTo(4 * step, step - step / 4);

  path.moveTo(4 * step + step / 3, 2 * step + step / 4);
  path.lineTo(4 * step, step - step / 4);

  return path;
}


QPainterPath SchematicScene::ccvsPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, 0);
  path.lineTo(2 * step, 0);
  path.lineTo(2 * step, step + step / 2);

  path.lineTo(step, 2 * step + step / 2);
  path.lineTo(2 * step, 3 * step + step / 2);
  path.lineTo(3 * step, 2 * step + step / 2);
  path.lineTo(2 * step, step + step / 2);

  path.moveTo(2 * step, 3 * step + step / 2);
  path.lineTo(2 * step, 5 * step);
  path.lineTo(0, 5 * step);

  path.moveTo(2 * step, 2 * step - step / 4);
  path.lineTo(2 * step, 2 * step + step / 4);

  path.moveTo(2 * step - step / 4, 2 * step);
  path.lineTo(2 * step + step / 4, 2 * step);

  path.moveTo(2 * step - step / 4, 3 * step);
  path.lineTo(2 * step + step / 4, 3 * step);

  path.moveTo(5 * step, 0);
  path.lineTo(4 * step, 0);
  path.lineTo(4 * step, 5 * step);
  path.lineTo(5 * step, 5 * step);

  // path.moveTo(4 * step - step / 3, 2 * step + step / 4);
  // path.lineTo(4 * step, step - step / 4);

  // path.moveTo(4 * step + step / 3, 2 * step + step / 4);
  // path.lineTo(4 * step, step - step / 4);
  
  path.moveTo(4 * step - step / 3, 3 * step - step / 4);
  path.lineTo(4 * step, 4 * step + step / 4);

  path.moveTo(4 * step + step / 3, 3 * step - step / 4);
  path.lineTo(4 * step, 4 * step + step / 4);

  return path;
}


QPainterPath SchematicScene::conductancePath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, step);
  path.lineTo(step, step);
  path.lineTo(step + step / 2, 0);
  path.lineTo(2 * step, 2 * step);
  path.lineTo(2 * step + step / 2, 0);
  path.lineTo(3 * step, 2 * step);
  path.lineTo(3 * step + step / 2, 0);
  path.lineTo(4 * step, step);
  path.lineTo(5 * step, step);

  return path;
}


QPainterPath SchematicScene::currentSourcePath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(step, 0);
  path.lineTo(step, step + step / 2);
  path.moveTo(step, 3 * step + step / 2);
  path.arcTo(
    QRectF(
      QPointF(0, step + step / 2),
      QPointF(2 * step, 3 * step + step / 2)
    ), -90, 360
  );
  path.lineTo(step, 5 * step);

  path.moveTo(step, 3 * step + step / 4);
  path.lineTo(step, 2 * step - step / 4);

  path.moveTo(step - step / 4, 2 * step + step / 4);
  path.lineTo(step, 2 * step - step / 4);

  path.moveTo(step + step / 4, 2 * step + step / 4);
  path.lineTo(step, 2 * step - step / 4);

  return path;
}


QPainterPath SchematicScene::inductorPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.addRect(0, 2 * step, 1E-99, 1E-99);
  path.moveTo(5 * step, step);
  path.lineTo(4 * step, step);
  path.arcTo(
    QRectF(
      QPointF(3 * step, 0),
      QPointF(4 * step, 2 * step)
    ), 0, 180
  );
  path.arcTo(
    QRectF(
      QPointF(2 * step, 0),
      QPointF(3 * step, 2 * step)
    ), 0, 180
  );
  path.arcTo(
    QRectF(
      QPointF(step, 0),
      QPointF(2 * step, 2 * step)
    ), 0, 180
  );
  path.lineTo(0, step);

  return path;
}


QPainterPath SchematicScene::opAmplPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(step, 0);
  path.lineTo(step, 4 * step);
  path.lineTo(4 * step, 2 * step);
  path.lineTo(step, 0);

  path.moveTo(0, step);
  path.lineTo(step, step);
  path.moveTo(0, 3 * step);
  path.lineTo(step, 3 * step);
  path.moveTo(4 * step, 2 * step);
  path.lineTo(5 * step, 2 * step);

  path.moveTo(step + step / 2 - step / 4, step);
  path.lineTo(step + step / 2 + step / 4, step);

  path.moveTo(step + step / 2 - step / 4, 3 * step);
  path.lineTo(step + step / 2 + step / 4, 3 * step);

  path.moveTo(step + step / 2, 3 * step - step / 4);
  path.lineTo(step + step / 2, 3 * step + step / 4);

  return path;
}


QPainterPath SchematicScene::resistorPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, step);
  path.lineTo(step, step);
  path.lineTo(step + step / 2, 0);
  path.lineTo(2 * step, 2 * step);
  path.lineTo(2 * step + step / 2, 0);
  path.lineTo(3 * step, 2 * step);
  path.lineTo(3 * step + step / 2, 0);
  path.lineTo(4 * step, step);
  path.lineTo(5 * step, step);

  return path;
}

QPainterPath SchematicScene::threeDInterfaceNodePath()
{
	QPainterPath path;
	uint step = GridStep;

	path.moveTo(0, 2 * step);
	path.lineTo(3*step, 2 * step);
	path.lineTo(1.5 * step, 0.5 * step);
	path.moveTo(3*step, 2 * step);
	path.lineTo(1.5 * step, 3.5 * step);

	path.addEllipse(2*step, 0, 2 * step, 4 * step);

	path.moveTo(3*step, 0);
	path.lineTo(4*step, 0);
	path.moveTo(3*step, 4 * step);
	path.lineTo(4*step, 4 * step);

	return path;
}

QPainterPath SchematicScene::prescribedPressureNodePath()
{
	QPainterPath path;
	uint step = GridStep;

	path.moveTo(0.5 * step, 0.5 * step);
	path.lineTo(1.5 * step, 1.5 * step);
	path.moveTo(0.5 * step, 1.5 * step);
	path.lineTo(1.5 * step, 0.5 * step);

	return path;
}

QPainterPath SchematicScene::vccsPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, 0);
  path.lineTo(2 * step, 0);
  path.lineTo(2 * step, step + step / 2);

  path.lineTo(step, 2 * step + step / 2);
  path.lineTo(2 * step, 3 * step + step / 2);
  path.lineTo(3 * step, 2 * step + step / 2);
  path.lineTo(2 * step, step + step / 2);

  path.moveTo(2 * step, 3 * step + step / 2);
  path.lineTo(2 * step, 5 * step);
  path.lineTo(0, 5 * step);

  path.moveTo(2 * step, 3 * step + step / 4);
  path.lineTo(2 * step, 2 * step - step / 4);

  path.moveTo(2 * step - step / 4, 2 * step + step / 4);
  path.lineTo(2 * step, 2 * step - step / 4);

  path.moveTo(2 * step + step / 4, 2 * step + step / 4);
  path.lineTo(2 * step, 2 * step - step / 4);

  path.moveTo(5 * step, 0);
  path.lineTo(4 * step, 0);
  path.moveTo(4 * step, 5 * step);
  path.lineTo(5 * step, 5 * step);

  path.moveTo(4 * step - step / 4, step);
  path.lineTo(4 * step + step / 4, step);

  path.moveTo(4 * step, 4 * step - step / 4);
  path.lineTo(4 * step, 4 * step + step / 4);

  path.moveTo(4 * step - step / 4, 4 * step);
  path.lineTo(4 * step + step / 4, 4 * step);

  return path;
}


QPainterPath SchematicScene::vcvsPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, 0);
  path.lineTo(2 * step, 0);
  path.lineTo(2 * step, step + step / 2);

  path.lineTo(step, 2 * step + step / 2);
  path.lineTo(2 * step, 3 * step + step / 2);
  path.lineTo(3 * step, 2 * step + step / 2);
  path.lineTo(2 * step, step + step / 2);

  path.moveTo(2 * step, 3 * step + step / 2);
  path.lineTo(2 * step, 5 * step);
  path.lineTo(0, 5 * step);

  path.moveTo(2 * step, 2 * step - step / 4);
  path.lineTo(2 * step, 2 * step + step / 4);

  path.moveTo(2 * step - step / 4, 2 * step);
  path.lineTo(2 * step + step / 4, 2 * step);

  path.moveTo(2 * step - step / 4, 3 * step);
  path.lineTo(2 * step + step / 4, 3 * step);

  path.moveTo(5 * step, 0);
  path.lineTo(4 * step, 0);
  path.moveTo(4 * step, 5 * step);
  path.lineTo(5 * step, 5 * step);

  path.moveTo(4 * step, step - step / 4);
  path.lineTo(4 * step, step + step / 4);

  path.moveTo(4 * step - step / 4, step);
  path.lineTo(4 * step + step / 4, step);

  path.moveTo(4 * step - step / 4, 4 * step);
  path.lineTo(4 * step + step / 4, 4 * step);

  return path;
}


QPainterPath SchematicScene::voltageSourcePath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(step, 0);
  path.lineTo(step, step + step / 2);
  path.moveTo(step, 3 * step + step / 2);
  path.arcTo(
    QRectF(
      QPointF(0, step + step / 2),
      QPointF(2 * step, 3 * step + step / 2)
    ), -90, 360
  );
  path.lineTo(step, 5 * step);

  path.moveTo(step - step / 4, 3 * step);
  path.lineTo(step + step / 4, 3 * step);

  path.moveTo(step, 2 * step - step / 4);
  path.lineTo(step, 2 * step + step / 4);

  path.moveTo(step - step / 4, 2 * step);
  path.lineTo(step + step / 4, 2 * step);

  return path;
}


QPainterPath SchematicScene::transformerPath()
{
  QPainterPath path;
  uint step = GridStep;

  path.moveTo(0, 0);
  path.lineTo(step, 0);

  path.moveTo(step, 5 * step);
  path.lineTo(step, 5 * step - step / 2);
  path.arcTo(
    QRectF(
      QPointF(0, 4 * step - step / 2),
      QPointF(2 * step, 5 * step - step / 2)
    ), -90, 180
  );
  path.arcTo(
    QRectF(
      QPointF(0, 3 * step - step / 2),
      QPointF(2 * step, 4 * step - step / 2)
    ), -90, 180
  );
  path.arcTo(
    QRectF(
      QPointF(0, 2 * step - step / 2),
      QPointF(2 * step, 3 * step - step / 2)
    ), -90, 180
  );
  path.arcTo(
    QRectF(
      QPointF(0, step - step / 2),
      QPointF(2 * step, 2 * step - step / 2)
    ), -90, 180
  );
  path.lineTo(step, 0);

  path.moveTo(step, 5 * step);
  path.lineTo(0, 5 * step);

  path.moveTo(2 * step + step / 3, step);
  path.lineTo(2 * step + step / 3, 4 * step);
  path.moveTo(3 * step - step / 3, step);
  path.lineTo(3 * step - step / 3, 4 * step);

  path.moveTo(5 * step, 0);
  path.lineTo(4 * step, 0);

  path.moveTo(4 * step, 5 * step);
  path.lineTo(4 * step, 5 * step - step / 2);
  path.arcTo(
    QRectF(
      QPointF(3 * step, 4 * step - step / 2),
      QPointF(5 * step, 5 * step - step / 2)
    ), -90, -180
  );
  path.arcTo(
    QRectF(
      QPointF(3 * step, 3 * step - step / 2),
      QPointF(5 * step, 4 * step - step / 2)
    ), -90, -180
  );
  path.arcTo(
    QRectF(
      QPointF(3 * step, 2 * step - step / 2),
      QPointF(5 * step, 3 * step - step / 2)
    ), -90, -180
  );
  path.arcTo(
    QRectF(
      QPointF(3 * step, step - step / 2),
      QPointF(5 * step, 2 * step - step / 2)
    ), -90, -180
  );
  path.lineTo(4 * step, 0);

  path.moveTo(4 * step, 5 * step);
  path.lineTo(5 * step, 5 * step);

  return path;
}


QPainterPath SchematicScene::mutualInductancePath()
{
  QPainterPath path;
  uint step = GridStep;

  path = transformerPath();
  path.addText(0, 3 * step, QFont(), "Lp");
  path.addText(4 * step, 3 * step, QFont(), "Ls");

  return path;
}


QtProperty* SchematicScene::itemProperties(QGraphicsItem* item)
{
  QVariant props = item->data(1);

  if(props == QVariant())
    return 0;

  return props.value<QtProperty*>();
}


SchematicScene::SupportedItemType SchematicScene::itemType(QGraphicsItem* item)
{
  QVariant type = item->data(0);

  if(type == QVariant())
    return NilItemType;

  bool ok;
  int intType = type.toInt(&ok);

  if(!ok)
    return NilItemType;

  return static_cast<SupportedItemType>(intType);
}


Item* SchematicScene::itemByType(SchematicScene::SupportedItemType type)
{
  Item* item;
  switch(type)
  {
  case CapacitorItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, GridStep));
      nodes.push_back(QPointF(5 * GridStep, GridStep));
      item = new Component(capacitorPath(), nodes);
      break;
    }
  case DiodeItemType:
  {
	  QList<QPointF> nodes;
	  nodes.push_back(QPointF(0, GridStep));
	  nodes.push_back(QPointF(5 * GridStep, GridStep));
	  item = new Component(diodePath(), nodes);
	  break;
  }
  case VolumeTrackingChamberItemType:
  {
	  QList<QPointF> nodes;
	  nodes.push_back(QPointF(0, GridStep));
	  nodes.push_back(QPointF(5 * GridStep, GridStep));
	  item = new Component(volumeTrackingPressureChamberPath(), nodes);
	  break;
  }
  case CCCSItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 5 * GridStep));
      nodes.push_back(QPointF(0, 0));
      nodes.push_back(QPointF(5 * GridStep, 0));
      nodes.push_back(QPointF(5 * GridStep, 5 * GridStep));
      item = new Component(cccsPath(), nodes);
      break;
    }
  case CCVSItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 0));
      nodes.push_back(QPointF(0, 5 * GridStep));
      nodes.push_back(QPointF(5 * GridStep, 5 * GridStep));
      nodes.push_back(QPointF(5 * GridStep, 0));
      item = new Component(ccvsPath(), nodes);
      break;
    }
  case ConductanceItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, GridStep));
      nodes.push_back(QPointF(5 * GridStep, GridStep));
      item = new Component(conductancePath(), nodes);
      break;
    }
  case CurrentSourceItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(GridStep, 5 * GridStep));
      nodes.push_back(QPointF(GridStep, 0));
      item = new Component(currentSourcePath(), nodes);
      break;
    }
  case InductorItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, GridStep));
      nodes.push_back(QPointF(5 * GridStep, GridStep));
      item = new Component(inductorPath(), nodes);
      break;
    }
  case OpAmplItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 3* GridStep));
      nodes.push_back(QPointF(0, GridStep));
      nodes.push_back(QPointF(5 * GridStep, 2 * GridStep));
      item = new Component(opAmplPath(), nodes);
      break;
    }
  case ResistorItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, GridStep));
      nodes.push_back(QPointF(5 * GridStep, GridStep));
      item = new Component(resistorPath(), nodes);
      break;
    }
  case threeDInterfaceNodeType:
	{
		item = new Component(threeDInterfaceNodePath(), QPointF(0, 2 * GridStep));
		break;
	}
  case prescribedPressureNodeType:
    {
		item = new Component(prescribedPressureNodePath(), QPointF(GridStep, GridStep));
		break;
    }
  case VCCSItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 5 * GridStep));
      nodes.push_back(QPointF(0, 0));
      nodes.push_back(QPointF(5 * GridStep, 5 * GridStep));
      nodes.push_back(QPointF(5 * GridStep, 0));
      item = new Component(vccsPath(), nodes);
      break;
    }
  case VCVSItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 0));
      nodes.push_back(QPointF(0, 5 * GridStep));
      nodes.push_back(QPointF(5 * GridStep, 0));
      nodes.push_back(QPointF(5 * GridStep, 5 * GridStep));
      item = new Component(vcvsPath(), nodes);
      break;
    }
  case VoltageSourceItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(GridStep, 0));
      nodes.push_back(QPointF(GridStep, 5 * GridStep));
      item = new Component(voltageSourcePath(), nodes);
      break;
    }
  case TransformerItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 0));
      nodes.push_back(QPointF(0, 5 * GridStep));
      nodes.push_back(QPointF(5 * GridStep, 0));
      nodes.push_back(QPointF(5 * GridStep, 5 * GridStep));
      item = new Component(transformerPath(), nodes);
      break;
    }
  case MutualInductanceItemType:
    {
      QList<QPointF> nodes;
      nodes.push_back(QPointF(0, 0));
      nodes.push_back(QPointF(0, 5 * GridStep));
      nodes.push_back(QPointF(5 * GridStep, 0));
      nodes.push_back(QPointF(5 * GridStep, 5 * GridStep));
      item = new Component(mutualInductancePath(), nodes);
      break;
    }
  case GroundItemType:
    {
      item = new Component(groundPath(), QPointF(GridStep, 0));
      break;
    }
  // case PortItemType:
  //   {
  //     item = new Component(portPath(), QPointF(GridStep, GridStep));
  //     break;
  //   }
  // case OutItemType:
  //   {
  //     item = new Component(outPath(), QPointF(0, GridStep));
  //     break;
  //   }
  // case VoltmeterItemType:
  //   {
  //     QList<QPointF> nodes;
  //     nodes.push_back(QPointF(GridStep, 0));
  //     nodes.push_back(QPointF(GridStep, 5 * GridStep));
  //     item = new Component(voltmeterPath(), nodes);
  //     break;
  //   }
  // case AmmeterItemType:
  //   {
  //     QList<QPointF> nodes;
  //     nodes.push_back(QPointF(GridStep, 0));
  //     nodes.push_back(QPointF(GridStep, 5 * GridStep));
  //     item = new Component(ammeterPath(), nodes);
  //     break;
  //   }
  case WireItemType:
    {
      item = new Wire;
      break;
    }
  case LabelItemType:
    {
      item = new Label;
      break;
    }
  // case UserDefItemType:
  //   {
  //     QList<QPointF> nodes;
  //     item = new Component(nilPath(), nodes);
  //     break;
  //   }
  default:
    item = 0;
  }

  if(item)
    item->setData(0, type);

  return item;
}


QString SchematicScene::itemNameByType(SchematicScene::SupportedItemType type)
{
  switch(type)
  {
  case GroundItemType:
    return QString(QObject::tr("Ground"));
  case threeDInterfaceNodeType:
	return QString(QObject::tr("threeDInterfaceNode"));
  case prescribedPressureNodeType:
	return QString(QObject::tr("Prescribed Pressure Node"));
  // case PortItemType:
  //   return QString(QObject::tr("Port"));
  // case OutItemType:
  //   return QString(QObject::tr("Out"));
  // case VoltmeterItemType:
  //   return QString(QObject::tr("Voltmeter"));
  // case AmmeterItemType:
  //   return QString(QObject::tr("Ammeter"));
  case WireItemType:
    return QString(QObject::tr("Wire"));
  case ResistorItemType:
    return QString(QObject::tr("Resistor"));
  case ConductanceItemType:
    return QString(QObject::tr("Conductance"));
  case InductorItemType:
    return QString(QObject::tr("Inductor"));
  case CapacitorItemType:
    return QString(QObject::tr("Capacitor"));
  case DiodeItemType:
	  return QString(QObject::tr("Diode"));
  case VolumeTrackingChamberItemType:
	  return QString(QObject::tr("Volume-Tracking Pressure Chamber"));
  case VoltageSourceItemType:
    return QString(QObject::tr("Voltage Source"));
  case TransformerItemType:
    return QString(QObject::tr("Ideal Transformer"));
  case MutualInductanceItemType:
    return QString(QObject::tr("Mutual Inductance"));
  case CurrentSourceItemType:
    return QString(QObject::tr("Current Source"));
  case VCVSItemType:
    return QString(QObject::tr("VCVS"));
  case CCVSItemType:
    return QString(QObject::tr("CCVS"));
  case VCCSItemType:
    return QString(QObject::tr("VCCS"));
  case CCCSItemType:
    return QString(QObject::tr("CCCS"));
  case OpAmplItemType:
    return QString(QObject::tr("Operational Amplifier"));
  // case UserDefItemType:
  //   return QString(QObject::tr("User Defined"));
  default:
    return QString(QObject::tr("Unknow"));
  }

  return QString();
}


QChar SchematicScene::itemIdByType(SchematicScene::SupportedItemType type)
{
  switch(type)
  {
  case ResistorItemType:
    return QChar('R');
  case threeDInterfaceNodeType:
	return QChar('T');
  case prescribedPressureNodeType:
	return QChar('P');
  case ConductanceItemType:
    return QChar('G');
  case InductorItemType:
    return QChar('L');
  case CapacitorItemType:
    return QChar('C');
  case DiodeItemType:
	return QChar('D');
  case VolumeTrackingChamberItemType:
	  return QChar('t');
  case VoltageSourceItemType:
    return QChar('V');
  case TransformerItemType:
    return QChar('n');
  case MutualInductanceItemType:
    return QChar('K');
  case CurrentSourceItemType:
    return QChar('I');
  case VCVSItemType:
    return QChar('E');
  case CCVSItemType:
    return QChar('Y');
  case VCCSItemType:
    return QChar('H');
  case CCCSItemType:
    return QChar('F');
  case OpAmplItemType:
    return QChar('A');
  // case UserDefItemType:
  //   return QChar('X');
  default:
    break;
  }

  return QChar();
}


QPainterPath SchematicScene::userDefPath(uint ports)
{
  QPainterPath path;
  uint step = GridStep;
  uint side = (ports + (ports % 2)) / 2;

  if(side) {
    path.moveTo(step, 0);
    path.lineTo(step, step * 2 * side);
    path.lineTo(5 * step, step * 2 * side);
    path.lineTo(5 * step, 0);
    path.lineTo(step, 0);
  } else {
    path.moveTo(0, 0);
    path.lineTo(0, 2 * step);
    path.lineTo(2 * step, 2 * step);
    path.lineTo(2 * step, 0);
    path.lineTo(0, 0);
    path.moveTo(0, 0);
    path.lineTo(2 * step, 2 * step);
    path.moveTo(2 * step, 0);
    path.lineTo(0, 2 * step);
  }

  for(uint i = 0; i < side; ++i) {
    path.moveTo(0, step * (2 * i + 1) + step / 3);
    path.lineTo(0, step * (2 * i + 1));
    path.lineTo(step, step * (2 * i + 1));
    path.addText(step * 4 / 3, step * (2 * i + 1) + step / 2,
        QFont("Times", 8, QFont::Light),
        QString::number(i + 1));
  }

  side -= ports % 2;
  for(uint i = 0; i < side; ++i) {
    path.moveTo(6 * step, step * (2 * i + 1) + step / 3);
    path.lineTo(6 * step, step * (2 * i + 1));
    path.lineTo(5 * step, step * (2 * i + 1));
    path.addText(4 * step - step / 3, step * (2 * i + 1) + step / 2,
        QFont("Times", 8, QFont::Light),
        QString::number(i + side + ports % 2 + 1));
  }

  return path;
}


QList<QPointF> SchematicScene::userDefNodes(uint ports)
{
  QList<QPointF> nodes;

  uint side = (ports + (ports % 2)) / 2;
  for(uint i = 0; i < side; ++i)
    nodes.push_back(QPointF(0, GridStep * (2 * i + 1)));

  side -= ports % 2;
  for(uint i = 0; i < side; ++i)
    nodes.push_back(QPointF(6 * GridStep, GridStep * (2 * i + 1)));

  return nodes;
}


SchematicScene::SchematicScene(QObject* parent): QGraphicsScene(parent) {
  setupProperties();
  init();
}


SchematicScene::~SchematicScene() {
  // delete itemized sub-circuits
  foreach(Item* item, userDefList_)
    delete item->data(101).value< QPointer<qsapecng::SchematicScene> >();

  delete undoRedoStack_;
}


QPointF SchematicScene::closestGridPoint(const QPointF& pos) const
{
  QPoint point = pos.toPoint();
  int x = point.x();
  int y = point.y();

  if(x < 0) x -= (GridStep >> 1) - 1;
  else x += (GridStep >> 1);
  x -= x % GridStep;

  if(y < 0) y -= (GridStep >> 1) - 1;
  else y += (GridStep >> 1);
  y -= y % GridStep;

  return QPointF(x,y);
}


void SchematicScene::setActiveItem(SchematicScene::SupportedItemType item)
{
  resetStatus();
  activeItem_ = item;
  if(hasActiveItem())
    foreach(QGraphicsView* view, views())
      view->setCursor(Qt::CrossCursor);
}

QList<Item*> SchematicScene::getStandardList() const
{
	return standardList_;
}

QList<Item*> SchematicScene::activeItems() const
{
  QList<Item*> items;

  items.append(portList_);
  items.append(threeDInterfaceNodeList_);
  items.append(groundList_);
  items.append(standardList_);
  items.append(userDefList_);
  items.append(wireList_);
  items.append(labelList_);
  if(out_)
    items.append(out_);

  return items;
}

QList<Component*> SchematicScene::getAllPrescribedPressureNodes() const
{
	QList<Component*> prescribedPressureNodes;
	// We just downcast these all here to the Component* that holds the
	// prescribedPressureNode, to avoid doing it later. 
	// Note that this is an abuse of existing structures;
	// we're using Component to hold what is really just
	// a pressure node. \todo consider refactoring this.
	foreach(Item* pressureNode, prescribedPressureNodesList_)
	{
		prescribedPressureNodes.append(static_cast<Component*>(pressureNode));
	}

	return prescribedPressureNodes;
}


QByteArray SchematicScene::registerUserDef(const SchematicScene& scene)
{
    std::ostringstream stream;
    sapecng::abstract_parser* parser = new SchematicSceneParser(scene);
    sapecng::abstract_builder* builder =
      sapecng::builder_factory::builder(sapecng::builder_factory::INFO, stream);
    
    if(builder)
      parser->parse(*builder);
    
    QByteArray str(stream.str().c_str());
    QByteArray md5 = QCryptographicHash::hash(str, QCryptographicHash::Md5);
    
    delete builder;
    delete parser;
    
    if(!userDefMap_.contains(md5))
      userDefMap_.insert(md5, stream.str());
    
    return md5;
}


std::string SchematicScene::queryUserDef(QByteArray md5)
{
  std::string rep;

  if(!userDefMap_.contains(md5))
    userDefMap_.value(md5);
  
  return rep;
}


// void SchematicScene::setUserDefRequest()
// {
//   if(0 == views().size())
//     return;
  
//   QString fileName = QFileDialog::getOpenFileName(views().at(0),
//       tr("Read file"), Settings().workspace(),
//       QString("%1;;%2")
//         .arg(tr("Info files (*.info)"))
//         .arg(tr("XML files (*.xml)"))
//     );

//   if(!fileName.isEmpty()) {
//     SchematicScene* scene = new SchematicScene(this);
//     sapecng::abstract_builder* builder = new SchematicSceneBuilder(*scene);
    
//     QFileInfo fileInfo(fileName);
//     std::ifstream in_file(QFile::encodeName(fileName));
//     sapecng::abstract_parser* parser =
//       sapecng::parser_factory::parser(
//         fileInfo.suffix().toStdString(), in_file);

//     if(parser)
//       parser->parse(*builder);

//     delete parser;
//     delete builder;
    
//     QByteArray md5 = registerUserDef(*scene);
//     int size = scene->size();

//     delete scene;
    
//     setActiveItem(SchematicScene::UserDefItemType);
//     userDefSessionRequested_ = true;
//     userDefSize_ = size;
//     userDefMD5_ = md5;
//   }
// }


void SchematicScene::addItems(QList<QGraphicsItem*> items)
{
  AddItems* command = new AddItems(this, items);
  undoRedoStack_->push(command);
}


void SchematicScene::addItems(QGraphicsItem* item)
{
  AddItems* command = new AddItems(this, item);
  undoRedoStack_->push(command);
}


void SchematicScene::addSupportedItem(QGraphicsItem* gItem, bool initialise)
{
  if(!gItem)
    return;

  Item* item = qgraphicsitem_cast<Item*>(gItem);
  if(!item) {
    addItem(gItem);
    return;
  }

  bool ok;
  QVariant data = item->data(0);
  if(!data.isNull()) {
    SupportedItemType type = (SupportedItemType) data.toInt(&ok);
    if(ok && type != NilItemType) {
      addItem(item);
      switch(type)
      {
	  case threeDInterfaceNodeType:
	  {
		  if (!threeDInterfaceNodeList_.contains(item))
		  {
			  threeDInterfaceNodeList_.push_back(item);
		  }
		  break;
	  }
      case GroundItemType:
        {
          if(!groundList_.contains(item))
            groundList_.push_back(item);
          break;
        }
      // case PortItemType:
      //   {
      //     if(!portList_.contains(item)) {
      //       portList_.push_back(item);

      //       static_cast<Component*>(item)->label()->setPlainText(
      //         QString::number(portList_.size()));
      //     }
      //     break;
      //   }
      // case OutItemType:
      // case VoltmeterItemType:
      // case AmmeterItemType:
      //   {
      //     if(!out_) {
      //       out_ = item;
      //     } else {
      //       out_->setOpacity(0.5);
      //       outList_.push_back(out_);
      //       out_ = item;
      //     }
      //     break;
      //   }
      case WireItemType:
        {
          if(!wireList_.contains(item))
            wireList_.push_back(item);
          break;
        }
      case LabelItemType:
        {
          if(!labelList_.contains(item))
            labelList_.push_back(item);
          break;
        }
	  case prescribedPressureNodeType:
	  {
		  if (!prescribedPressureNodesList_.contains(item)) {
			  
			  static_cast<Component*>(item)->setThisIsReallyJustAPrescribedPressureNode();

			  prescribedPressureNodesList_.push_back(item);
			  QtProperty* name = stringManager_->addProperty(itemNameByType(type));

			  if (initialise) {
				  stringManager_->setValue(name,
					  itemIdByType(type) + QString::number(++itemCntMap_[type]));
			  }
			  else {
				  stringManager_->setValue(name, tr("noname"));
			  }

			  item->setData(1, QVariant::fromValue<QtProperty*>(name));
			  static_cast<Component*>(item)->label()->setProperty(name);
			  
			  connect(
				  stringManager_,
				  SIGNAL(valueChanged(QtProperty*, const QString&)),
				  static_cast<Component*>(item)->label(),
				  SLOT(valueChanged(QtProperty*, const QString&))
				  );

			  QtProperty* prescribedPressure = doublePropertyManager_->addProperty(tr(predefinedStrings::prescribedPressureParameterValueName));

			  doublePropertyManager_->setValue(prescribedPressure, 0.0);
			  doublePropertyManager_->setSingleStep(prescribedPressure, 0.01);
			  doublePropertyManager_->setDecimals(prescribedPressure, 13);

			  //QRegExp sn("[+]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
			  //stringManager_->setRegExp(prescribedPressure, sn);
			  name->addSubProperty(prescribedPressure);

			  // Drop-down menu for the nodal controller types:
			  QtVariantProperty* prop = managerForAllDropdownBoxProperties_->addProperty(QtVariantPropertyManager::enumTypeId(), tr(predefinedStrings::nodeControlTypeFieldName));
        QStringList listOfOptions;
        listOfOptions << predefinedStrings::nodeControlTypes[predefinedStrings::nodeControlTypeCodes::ControlNone];
        listOfOptions << predefinedStrings::nodeControlTypes[predefinedStrings::nodeControlTypeCodes::ControlCustomPython];
        listOfOptions << predefinedStrings::nodeControlTypes[predefinedStrings::nodeControlTypeCodes::ControlPeriodicPrescribedPressure];
			  prop->setAttribute("enumNames", listOfOptions);
			  prop->setValue(0); // "Suggestion"
		name->addSubProperty(prop);

        // Text box to enter the Python script name, if the controller type is customPython:
			  QtProperty* customDataFileName = stringManager_->addProperty(tr(predefinedStrings::additionalDataFileFieldName));
        stringManager_->setValue(customDataFileName, tr(predefinedStrings::pythonScriptnamePlaceholder));

		// Grey out the box initially, until the user selects custom python control:
		customDataFileName->setEnabled(false);

		// Qt 5 allows use of lambdas as signals. This is awesome.
		// This connection does the greying out of the python script name field when the
		// type of control is not set to custom python.
		//
		// Note that we must capture "this" in order to access the stringManager_
		// This is because lambdas can only capture things in the (immediate) enclosing scope,
		// and in the enclosing scope, access to stringManager_ is really via this->stringManager_.
		QObject::connect(managerForAllDropdownBoxProperties_, &QtVariantPropertyManager::valueChanged, 
			[prop, customDataFileName, this](QtProperty* changedProp, const QVariant& newValue){
			if (changedProp != prop) {
				return;
			}

			auto controlType = static_cast<predefinedStrings::nodeControlTypeCodes::nodeControlTypeCodes>(newValue.toInt());

      // Switch the placeholder text in the file name box, depending
      // on whether the file name needs to be a Python script or a pressure-time two-column
      // data file.
	  std::string userProvidedDataFileName = this->stringManager_->value(customDataFileName).toStdString();
	  bool stillIsDefaultPythonScriptnamePlaceholder = (userProvidedDataFileName == std::string(predefinedStrings::pythonScriptnamePlaceholder));
	  bool stillIsDefaultPressureDataFilePlaceholder = (userProvidedDataFileName == std::string(predefinedStrings::timePressureDataFilePlaceholder));
      bool noFileNameSpecifiedYet = (stillIsDefaultPythonScriptnamePlaceholder || stillIsDefaultPressureDataFilePlaceholder);
			
      bool pythonScriptNameRequired = (controlType == predefinedStrings::nodeControlTypeCodes::ControlCustomPython);
      if (pythonScriptNameRequired && noFileNameSpecifiedYet)
      {
        this->stringManager_->setValue(customDataFileName, tr(predefinedStrings::pythonScriptnamePlaceholder));    
      }
      bool dataFileNameRequired = (controlType == predefinedStrings::nodeControlTypeCodes::ControlPeriodicPrescribedPressure);
      if (dataFileNameRequired && noFileNameSpecifiedYet)
      {
        this->stringManager_->setValue(customDataFileName, tr(predefinedStrings::timePressureDataFilePlaceholder));
      }

      // Enable or disable (grey out) the file name field, depending on whether it is needed:
	    bool fileNameRequired = (pythonScriptNameRequired || dataFileNameRequired);
			customDataFileName->setEnabled(fileNameRequired);
		});
    
    
    

        name->addSubProperty(customDataFileName);

			  properties_->addSubProperty(typeRootMap_[AllNodesType]);

			  static_cast<Component*>(item)->label()->setPlainText(
				  stringManager_->value(name));

			  typeRootMap_[type]->addSubProperty(name);
			  properties_->addSubProperty(typeRootMap_[type]);
		  }

		  break;
	  }
	  case VolumeTrackingChamberItemType:
	  {
		  if (!standardList_.contains(item)) {
			  standardList_.push_back(item);
			  QtProperty* propertyToAddVolumeParameterTo = setupCrimsonBctComponentCommon(item, type, initialise);
			  addInitialVolumeParameterSetter(propertyToAddVolumeParameterTo);
		  }
		  break;
	  }
      case CapacitorItemType:
	  case DiodeItemType:
      case ConductanceItemType:
      case InductorItemType:
	  case ResistorItemType:
	  {
		  if (!standardList_.contains(item)) {
			  standardList_.push_back(item);
			  setupCrimsonBctComponentCommon(item, type, initialise);
		  }
		break;
	  }
      case CCCSItemType:
      case CCVSItemType:
      case VCCSItemType:
      case VCVSItemType:
      case CurrentSourceItemType:
      case VoltageSourceItemType:
      case TransformerItemType:
        {
			if (!standardList_.contains(item)) {
				standardList_.push_back(item);
				QtProperty* name = stringManager_->addProperty(itemNameByType(type));
				
				if (initialise) {
					stringManager_->setValue(name, itemIdByType(type) + QString::number(++itemCntMap_[type]));
				}
				else {
					stringManager_->setValue(name, tr("noname"));
				}
				setupItemLabel(item, name);

				QtVariantProperty* prop;
				QtProperty* customDataFileName;
              QtProperty* pComponentParameterValue = stringManager_->addProperty(tr(predefinedStrings::genericComponentParameterName));

              stringManager_->setValue(pComponentParameterValue, "1.0");
              QRegExp sn("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
              stringManager_->setRegExp(pComponentParameterValue, sn);

              name->addSubProperty(pComponentParameterValue);
			        name->addSubProperty(prop);
			        name->addSubProperty(customDataFileName);

			  setLabelAndAddComponentToHierarchy(item, name, type);
          }
          break;
        }
	  case OpAmplItemType:
	  {
		  if (!standardList_.contains(item)) {
			  standardList_.push_back(item);
			  QtProperty* name = stringManager_->addProperty(itemNameByType(type));

			  if (initialise) {
				  stringManager_->setValue(name, itemIdByType(type) + QString::number(++itemCntMap_[type]));
			  }
			  else {
				  stringManager_->setValue(name, tr("noname"));
			  }
			  setupItemLabel(item, name);
			  setLabelAndAddComponentToHierarchy(item, name, type);
		  }
		  break;
	  }
	  case MutualInductanceItemType:
	  {
		  if (!standardList_.contains(item)) {
			  standardList_.push_back(item);
			  QtProperty* name = stringManager_->addProperty(itemNameByType(type));

			  if (initialise) {
				  stringManager_->setValue(name, itemIdByType(type) + QString::number(++itemCntMap_[type]));
			  }
			  else {
				  stringManager_->setValue(name, tr("noname"));
			  }
			  setupItemLabel(item, name);

				QtVariantProperty* prop;
				QtProperty* customDataFileName;
				QtProperty* pComponentParameterValue = stringManager_->addProperty(tr("M"));

				stringManager_->setValue(pComponentParameterValue, "1.0");
				QRegExp sn("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
				stringManager_->setRegExp(pComponentParameterValue, sn);

				name->addSubProperty(pComponentParameterValue);
				name->addSubProperty(prop);
				name->addSubProperty(customDataFileName);

				setupMutualInductanceProperties(name);

				setLabelAndAddComponentToHierarchy(item, name, type);
		  }
		  break;
	  }
      // case UserDefItemType:
      //   {
      //     if(!userDefList_.contains(item)) {
      //       userDefList_.push_back(item);

      //       QtProperty* name =
      //         stringManager_->addProperty(itemNameByType(type));

      //       stringManager_->setValue(name,
      //           itemIdByType(type) + QString::number(++itemCntMap_[type]));

      //       item->setData(1, QVariant::fromValue<QtProperty*>(name));
      //       static_cast<Component*>(item)->label()->setProperty(name);
      //       connect(
      //         stringManager_,
      //         SIGNAL(valueChanged(QtProperty*, const QString&)),
      //         static_cast<Component*>(item)->label(),
      //         SLOT(valueChanged(QtProperty*, const QString&))
      //       );

      //       QPointer<qsapecng::SchematicScene> rep =
      //         item->data(101)
      //           .value< QPointer<qsapecng::SchematicScene> >();

      //       name->addSubProperty(rep->properties());
      //       rep->initializeBrowser(browser_);

      //       static_cast<Component*>(item)->label()->setPlainText(
      //         stringManager_->value(name));

      //       typeRootMap_[type]->addSubProperty(name);
      //       properties_->addSubProperty(typeRootMap_[type]);
      //     }
      //     break;
      //   }
      default:
        break;
      }
    }
  }
}

QtProperty* SchematicScene::setupCrimsonBctComponentCommon(Item* const item, SupportedItemType const type, const bool initialise)
{
		QtProperty* name = stringManager_->addProperty(itemNameByType(type));

		if (initialise) {
			stringManager_->setValue(name, itemIdByType(type) + QString::number(++itemCntMap_[type]));
		}
		else {
			stringManager_->setValue(name, tr("noname"));
		}

		setupItemLabel(item, name);

		QtProperty* pComponentParameterValue = stringManager_->addProperty(tr(predefinedStrings::genericComponentParameterName));

		// Component parameter value box (resistance, compliance, etc..)
		stringManager_->setValue(pComponentParameterValue, "1.0");
		QRegExp sn("[+]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
		stringManager_->setRegExp(pComponentParameterValue, sn);

		QtVariantProperty* prop;
		QtProperty* customDataFileName;
		setupComponentControlSpecification(prop, customDataFileName);

		name->addSubProperty(pComponentParameterValue);
		name->addSubProperty(prop);
		name->addSubProperty(customDataFileName);

		setLabelAndAddComponentToHierarchy(item, name, type);
		
		return name; // return it so we can add additional properties if necessary for the component type
}

void SchematicScene::addInitialVolumeParameterSetter(QtProperty* const name)
{
	QtProperty* pInitialVolumeParameterValue = stringManager_->addProperty(tr(predefinedStrings::initialVolumeParameterName));

	// Component parameter value box (resistance, compliance, etc..)
	stringManager_->setValue(pInitialVolumeParameterValue, "1.0");
	QRegExp sn("[+]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
	stringManager_->setRegExp(pInitialVolumeParameterValue, sn);


	name->addSubProperty(pInitialVolumeParameterValue);
}

void SchematicScene::setupItemLabel(Item* const item, QtProperty* const name)
{
	item->setData(1, QVariant::fromValue<QtProperty*>(name));
	static_cast<Component*>(item)->label()->setProperty(name);
	
	connect(
		stringManager_,
		SIGNAL(valueChanged(QtProperty*, const QString&)),
		static_cast<Component*>(item)->label(),
		SLOT(valueChanged(QtProperty*, const QString&))
		);
}

void SchematicScene::setupComponentControlSpecification(QtVariantProperty*& prop, QtProperty*& customDataFileName)
{
	// Drop-down box for the component controller type:
	prop = managerForAllDropdownBoxProperties_->addProperty(QtVariantPropertyManager::enumTypeId(), tr(predefinedStrings::componentControlTypeFieldName));
	QStringList dropDownOptions;
	dropDownOptions << predefinedStrings::componentControlTypes[predefinedStrings::componentControlTypeCodes::ControlNone];
	dropDownOptions << predefinedStrings::componentControlTypes[predefinedStrings::componentControlTypeCodes::ControlCustomPython];
	dropDownOptions << predefinedStrings::componentControlTypes[predefinedStrings::componentControlTypeCodes::ControlPeriodicPrescribedFlow];
	dropDownOptions << predefinedStrings::componentControlTypes[predefinedStrings::componentControlTypeCodes::ControlLVElastance];
	prop->setAttribute("enumNames", dropDownOptions);
	prop->setValue(0); // "Suggestion"

	customDataFileName = stringManager_->addProperty(tr(predefinedStrings::additionalDataFileFieldName));
	stringManager_->setValue(customDataFileName, tr(predefinedStrings::pythonScriptnamePlaceholder));

	// Grey out the box initially, until the user selects custom python control:
	customDataFileName->setEnabled(false);

	// Qt 5 allows use of lambdas as signals. This is awesome.
	// This connection does the greying out of the python script name field when the
	// type of control is not set to custom python.
	//
	// Note that we must capture "this" in order to access the stringManager_
	// This is because lambdas can only capture things in the (immediate) enclosing scope,
	// and in the enclosing scope, access to stringManager_ is really via this->stringManager_.
	QObject::connect(managerForAllDropdownBoxProperties_, &QtVariantPropertyManager::valueChanged,
		[prop, customDataFileName, this](QtProperty* changedProp, const QVariant& newValue) {
		if (changedProp != prop) {
			return;
		}

		auto controlType = static_cast<predefinedStrings::componentControlTypeCodes::componentControlTypeCodes>(newValue.toInt());

		// Switch the placeholder text in the file name box, depending
		// on whether the file name needs to be a Python script or a pressure-time two-column
		// data file.
		std::string currentDatafileName = this->stringManager_->value(customDataFileName).toStdString();
		std::string pythonScriptnamePlaceholder(predefinedStrings::pythonScriptnamePlaceholder);
		bool stillIsDefaultPythonScriptnamePlaceholder = (pythonScriptnamePlaceholder == currentDatafileName);

		std::string currentTextOfFileName = this->stringManager_->value(customDataFileName).toStdString();
		std::string timeFilenamePlaceholder(predefinedStrings::timeFlowDataFilePlaceholder);
		bool stillIsDefaultFlowDataFilePlaceholder = (currentTextOfFileName == timeFilenamePlaceholder);
		bool noFileNameSpecifiedYet = (stillIsDefaultPythonScriptnamePlaceholder || stillIsDefaultFlowDataFilePlaceholder);

		bool pythonScriptNameRequired = (controlType == predefinedStrings::componentControlTypeCodes::ControlCustomPython);
		if (pythonScriptNameRequired && noFileNameSpecifiedYet)
		{
			this->stringManager_->setValue(customDataFileName, tr(predefinedStrings::pythonScriptnamePlaceholder));
		}
		bool dataFileNameRequired = (controlType == predefinedStrings::componentControlTypeCodes::ControlPeriodicPrescribedFlow);
		if (dataFileNameRequired && noFileNameSpecifiedYet)
		{
			this->stringManager_->setValue(customDataFileName, tr(predefinedStrings::timeFlowDataFilePlaceholder));
		}

		// Enable or disable (grey out) the file name field, depending on whether it is needed:
		bool fileNameRequired = (pythonScriptNameRequired || dataFileNameRequired);
		customDataFileName->setEnabled(fileNameRequired);
	});
}

void SchematicScene::setupMutualInductanceProperties(QtProperty* const name)
{
	QtProperty* lpn = stringManager_->addProperty(tr("lp:name"));
	QtProperty* lpv = stringManager_->addProperty(tr("lp:value"));
	QtProperty* lsn = stringManager_->addProperty(tr("ls:name"));
	QtProperty* lsv = stringManager_->addProperty(tr("ls:value"));

	stringManager_->setValue(lpn, name->valueText() + "_Lp");
	stringManager_->setValue(lsn, name->valueText() + "_Ls");

	QRegExp sn("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
	stringManager_->setRegExp(lpv, sn);
	stringManager_->setValue(lpv, "1.0");
	stringManager_->setRegExp(lsv, sn);
	stringManager_->setValue(lsv, "1.0");

	name->addSubProperty(lpn);
	name->addSubProperty(lpv);
	name->addSubProperty(lsn);
	name->addSubProperty(lsv);
}

void SchematicScene::setLabelAndAddComponentToHierarchy(Item* const item, QtProperty* const name, SupportedItemType const type)
{
	static_cast<Component*>(item)->label()->setPlainText(stringManager_->value(name));

	typeRootMap_[type]->addSubProperty(name);
	properties_->addSubProperty(typeRootMap_[type]);
}

void SchematicScene::removeSupportedItem(QGraphicsItem* gItem)
{
  if(!gItem)
    return;

  Item* item = qgraphicsitem_cast<Item*>(gItem);
  if(!item) {
    if(gItem->scene() == this)
      removeItem(gItem);
    return;
  }

  bool ok;
  QVariant data = item->data(0);
  if(!data.isNull()) {
    SupportedItemType type = (SupportedItemType) data.toInt(&ok);
    if(ok && type != NilItemType) {

      if(item->scene() == this)
        removeItem(item);

      switch(type)
      {
	  case threeDInterfaceNodeType:
	  {
		  threeDInterfaceNodeList_.removeAll(item);
		  break;
	  }
      case GroundItemType:
        {
          groundList_.removeAll(item);
          break;
        }
      // case PortItemType:
      //   {
      //     portList_.removeAll(item);
      //     for(QList<Item*>::size_type i = 0; i < portList_.size(); ++i)
      //       static_cast<Component*>(portList_.at(i))->label()->setPlainText(
      //         QString::number(i + 1));
      //     break;
      //   }
      // case OutItemType:
      // case VoltmeterItemType:
      // case AmmeterItemType:
      //   {
      //     if(out_ == item) {
      //       if(outList_.isEmpty()) {
      //         out_ = 0;
      //       } else {
      //         out_ = outList_.back();
      //         outList_.pop_back();
      //         out_->setOpacity(1.0);
      //       }
      //     } else {
      //       if(outList_.contains(item)) {
      //         item->setOpacity(1.0);
      //         outList_.removeAll(item);
      //       }
      //     }
      //     break;
      //   }
      case WireItemType:
        {
          wireList_.removeAll(item);
          break;
        }
      case LabelItemType:
        {
          labelList_.removeAll(item);
          break;
        }
	  case prescribedPressureNodeType:
	  {
		  if (prescribedPressureNodesList_.contains(item))
		  {
			  if (itemProperties(item))
			  {
				  typeRootMap_[type]->removeSubProperty(itemProperties(item));
				  if (typeRootMap_[type]->subProperties().size() == 0)
				  {
					  properties_->removeSubProperty(typeRootMap_[type]);
				  }
			  }
			  prescribedPressureNodesList_.removeAll(item);
		  }
		  break;
	  }
      case CapacitorItemType:
	  case DiodeItemType:
	  case VolumeTrackingChamberItemType:
      case ConductanceItemType:
      case InductorItemType:
      case ResistorItemType:
      case CCCSItemType:
      case CCVSItemType:
      case VCCSItemType:
      case VCVSItemType:
      case CurrentSourceItemType:
      case VoltageSourceItemType:
      case TransformerItemType:
      case MutualInductanceItemType:
      case OpAmplItemType:
        {
          if(standardList_.contains(item)) {
            if(itemProperties(item)) {
              typeRootMap_[type]->removeSubProperty(itemProperties(item));
              if(typeRootMap_[type]->subProperties().size() == 0)
                properties_->removeSubProperty(typeRootMap_[type]);
            }

            standardList_.removeAll(item);
          }
          break;
        }
      // case UserDefItemType:
      //   {
      //     if(userDefList_.contains(item)) {
      //       if(itemProperties(item)) {
      //         typeRootMap_[type]->removeSubProperty(itemProperties(item));
      //         if(typeRootMap_[type]->subProperties().size() == 0)
      //           properties_->removeSubProperty(typeRootMap_[type]);
      //       }

      //       userDefList_.removeAll(item);
      //     }
      //     break;
      //   }
      default:
        break;
      }

    }
  }
}


void SchematicScene::clearSupportedItem(QGraphicsItem* gItem)
{
  if(!gItem)
    return;

  Item* item = qgraphicsitem_cast<Item*>(gItem);
  if(!item) {
    if(gItem->scene() == this)
      removeItem(gItem);
    return;
  }

  bool ok;
  QVariant data = item->data(0);
  if(!data.isNull()) {
    SupportedItemType type = (SupportedItemType) data.toInt(&ok);
    if(ok && type != NilItemType) {

      if(item->scene() == this)
        removeItem(item);

      switch(type)
      {
	  case threeDInterfaceNodeType:
	  {
		  threeDInterfaceNodeList_.removeAll(item);
		  break;
	  }
      case GroundItemType:
        {
          groundList_.removeAll(item);
          break;
        }
      // case PortItemType:
      //   {
      //     portList_.removeAll(item);
      //     for(QList<Item*>::size_type i = 0; i < portList_.size(); ++i)
      //       static_cast<Component*>(portList_.at(i))->label()->setPlainText(
      //         QString::number(i + 1));
      //     break;
      //   }
      // case OutItemType:
      // case VoltmeterItemType:
      // case AmmeterItemType:
      //   {
      //     if(out_ == item) {
      //       if(outList_.isEmpty()) {
      //         out_ = 0;
      //       } else {
      //         out_ = outList_.back();
      //         outList_.pop_back();
      //         out_->setOpacity(1.0);
      //       }
      //     } else {
      //       if(outList_.contains(item)) {
      //         item->setOpacity(1.0);
      //         outList_.removeAll(item);
      //       }
      //     }
      //     break;
      //   }
      case WireItemType:
        {
          wireList_.removeAll(item);
          break;
        }
      case LabelItemType:
        {
          labelList_.removeAll(item);
          break;
        }
      case CapacitorItemType:
	  case DiodeItemType:
	  case VolumeTrackingChamberItemType:
      case ConductanceItemType:
      case InductorItemType:
      case ResistorItemType:
      case CCCSItemType:
      case CCVSItemType:
      case VCCSItemType:
      case VCVSItemType:
      case CurrentSourceItemType:
      case VoltageSourceItemType:
      case TransformerItemType:
      case MutualInductanceItemType:
      case OpAmplItemType:
        {
          if(standardList_.contains(item)) {
            QtProperty* props = itemProperties(item);
            if(props) {
              typeRootMap_[type]->removeSubProperty(itemProperties(item));
              if(typeRootMap_[type]->subProperties().size() == 0)
                properties_->removeSubProperty(typeRootMap_[type]);

              foreach(QtProperty* prop, props->subProperties())
                delete prop;

              delete props;
            }

            standardList_.removeAll(item);
          }
          break;
        }
      // case UserDefItemType:
      //   {
      //     if(userDefList_.contains(item)) {
      //       if(itemProperties(item)) {
      //         typeRootMap_[type]->removeSubProperty(itemProperties(item));
      //         if(typeRootMap_[type]->subProperties().size() == 0)
      //           properties_->removeSubProperty(typeRootMap_[type]);
      //       }

      //       userDefList_.removeAll(item);
      //     }
      //     break;
      //   }
      default:
        break;
      }

    }
  }
}


void SchematicScene::setWireSessionRequest(bool connectedWire)
{
  resetStatus();

  wireSessionRequested_ = true;
  horizontalFirst_ = true;
  hasValidFromPoint_ = false;
  connectedWire_ = connectedWire;
  preWireItem_ = addLine(QLineF(fromPoint_, fromPoint_), wirePen_);
  postWireItem_ = addLine(QLineF(fromPoint_, fromPoint_), wirePen_);
  preWireItem_->setVisible(false);
  postWireItem_->setVisible(false);

  foreach(QGraphicsView* view, views())
    view->setCursor(Qt::CrossCursor);
}


void SchematicScene::joinWires(Wire* w1, Wire* w2)
{
  if(!w1 || !w2)
    return;

  JoinWires* command = new JoinWires(w1, w2);
  undoRedoStack_->push(command);
}


void SchematicScene::modifyWire(Wire* wire)
{
  // if(!hasActiveItem() && !hasUserDefReq() && !hasPendingWire() && wire) {
  if(!hasActiveItem() && !hasPendingWire() && wire) {
    resetStatus();

    wireSessionRequested_ = true;
    horizontalFirst_ = (wire->orientation() == Qt::Horizontal);
    fromPoint_ = wire->mapToScene(wire->fromPoint());
    QPointF toPoint = wire->mapToScene(wire->toPoint());
    hasValidFromPoint_ = true;
    connectedWire_ = wire->isJunctionsConnected();
    preWireItem_ = addLine(QLineF(fromPoint_, toPoint), wirePen_);
    postWireItem_ = addLine(QLineF(fromPoint_, fromPoint_), wirePen_);

    wireInProgress_ = wire;
    wire->setVisible(false);
    preWireItem_->setVisible(true);
    postWireItem_->setVisible(false);

    foreach(QGraphicsView* view, views())
      view->setCursor(Qt::CrossCursor);
  }
}


void SchematicScene::createLabel()
{
  resetStatus();

  AddItems* command = new AddItems(this,
    itemByType(LabelItemType), QPointF(3 * GridStep, 3 * GridStep));
  undoRedoStack_->push(command);
}


void SchematicScene::moveSelectedItems(QPointF offset)
{
  QList<QGraphicsItem*> items = selectedItems();
  if(items.isEmpty())
    return;

  MoveItems* command = new MoveItems(this, items, offset);
  undoRedoStack_->push(command);
}


void SchematicScene::cutSelectedItems()
{
  undoRedoStack_->beginMacro(tr("Cut item(s)"));
  copySelectedItems();
  binSelectedItems();
  undoRedoStack_->endMacro();
}


void SchematicScene::copySelectedItems()
{
  std::ostringstream stream;
  sapecng::abstract_builder* out =
    sapecng::builder_factory::builder(sapecng::builder_factory::INFO, stream);

  SchematicSceneParser* parser =
    new SchematicSceneParser(*this, selectedItems());

  parser->parse(*out);
  
  delete parser;
  delete out;

  QApplication::clipboard()->setText(
    QString::fromStdString(stream.str()));
}


void SchematicScene::pasteItems()
{
  SchematicSceneBuilder* out = 
    new SchematicSceneBuilder(*this, lastMousePressPos_);

  std::istringstream stream;
  stream.str(QApplication::clipboard()->text().toStdString());
  sapecng::abstract_parser* parser =
    sapecng::parser_factory::parser(sapecng::parser_factory::INFO, stream);

  undoRedoStack_->beginMacro(QObject::tr("Add item(s)"));
  parser->parse(*out);
  undoRedoStack_->endMacro();
  delete parser;
  delete out;
}


void SchematicScene::rotateSelectedItems()
{
  QList<QGraphicsItem*> items = selectedItems();
  if(items.isEmpty())
    return;

  RotateItems* command = new RotateItems(items);
  undoRedoStack_->push(command);
}


void SchematicScene::mirrorSelectedItems()
{
  QList<QGraphicsItem*> items = selectedItems();
  if(items.isEmpty())
    return;

  MirrorItems* command = new MirrorItems(items);
  undoRedoStack_->push(command);
}


void SchematicScene::binSelectedItems()
{
  QList<QGraphicsItem*> items = selectedItems();
  if(items.isEmpty())
    return;

  RemoveItems* command = new RemoveItems(this, items);
  undoRedoStack_->push(command);
}


void SchematicScene::bringToFrontSelectedItem()
{
  QGraphicsItem* item = itemAt(lastMousePressPos_, QTransform());
  if(!item)
    return;

  clearSelection();
  item->setSelected(true);
  BringToFrontItem* command = new BringToFrontItem(item);
  undoRedoStack_->push(command);
}


void SchematicScene::sendToBackSelectedItem()
{
  QGraphicsItem* item = itemAt(lastMousePressPos_, QTransform());
  if(!item)
    return;

  clearSelection();
  item->setSelected(true);
  SendToBackItem* command = new SendToBackItem(item);
  undoRedoStack_->push(command);
}


void SchematicScene::clearSchematicScene()
{
  QList<QGraphicsItem*> itemList = items();
  if(itemList.isEmpty())
    return;

  undoRedoStack_->beginMacro("Clear schematic scene");
  RemoveItems* command = new RemoveItems(this, itemList);
  undoRedoStack_->push(command);
  undoRedoStack_->endMacro();
}


void SchematicScene::initializeBrowser(QtAbstractPropertyBrowser* browser)
{
	  browser_ = browser;
	  browser_->setFactoryForManager(boolManager_, checkBoxFactory_);
	  browser_->setFactoryForManager(stringManager_, lineEditFactory_);
	  browser_->setFactoryForManager(doublePropertyManager_, doubleSpinboxFactory_);
	  browser_->setFactoryForManager(managerForAllDropdownBoxProperties_, variantFactory_);
}


QVector<int> SchematicScene::ports() {
  QVector<int> vector;
  foreach(Item* item, portList_)
    vector += static_cast<Component*>(item)->nodes();
  
  return vector;
}

void SchematicScene::passPrescribedPressureNodesTheirNodeIndices()
{
	// Working component by component...
	foreach(Item* component, standardList_)
	{
		// ...check each component's nodes to see if it intersects with a prescribed presure graphics object node:
		// (i.e. whether that node has a prescribed pressure.
		// If so, copy the node index from the component's node to the 
		// prescribed pressure node object (because they're both annotations to the same conceptual node)
		foreach(Item* prescribedPressureNode, prescribedPressureNodesList_)
		{
			int collidingNodeIndex;
			bool componentHasThisPrescribedPressureNode = static_cast<Component*>(component)->hasPrescribedPressureNode(static_cast<Component*>(prescribedPressureNode), collidingNodeIndex);
			
			if (componentHasThisPrescribedPressureNode)
			{
				static_cast<Component*>(prescribedPressureNode)->setIndexIfThisIsAPrescribedPressureNode(collidingNodeIndex);
			}
		}
	}
}

void SchematicScene::detectComponentAt3DInterface()
{
	// We only support exactly one 3D interface node for now:
	if (threeDInterfaceNodeList_.size() != 1)
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", "Must have exactly one 3D Interface node!");
		messageBox.setFixedSize(500, 200);
	}
	// Only one component may connect to the 3D interface, so we count how many we find:
	int componentsFoundAt3DInterface = 0;
	if (threeDInterfaceNodeList_.size() > 0)
	{
		foreach(Item* component, standardList_)
		{
			// First, we begin by assuming the component is not at the 3D Interface
			// This is in case the user has moved the 3D interface tag since
			// the last call to this function.
			static_cast<Component*>(component)->resetIsNotAt3DInterface();

			// Check for collisions between the node at the 3D interface and
			// the nodes of each component. This lets us find which component
			// is at the 3D interface.
			if (static_cast<Component*>(component)->connectsTo3DInterfaceNode(static_cast<Component*>(threeDInterfaceNodeList_.last())))
			{
				static_cast<Component*>(component)->setIsAt3DInterface();
				componentsFoundAt3DInterface++;
			}
		}
	}

	if (componentsFoundAt3DInterface != 1)
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", "You must have exactly one component (and zero wires) connected to 3D interface!");
		messageBox.setFixedSize(500, 200);
	}
}

void SchematicScene::assignNodes(int seed)
{
  resetStatus();

  foreach(Item* item, groundList_)
    //static_cast<Component*>(item)->propagate(SchematicScene::Ground);
	seed = static_cast<Component*>(item)->propagate(seed);
  foreach(Item* item, threeDInterfaceNodeList_)
	  seed = static_cast<Component*>(item)->propagate(seed);
  /*foreach(Item* item, wireList_)
    seed = static_cast<Wire*>(item)->propagate(seed);*/
  foreach(Item* item, standardList_)
    seed = static_cast<Component*>(item)->propagate(seed);
  foreach(Item* item, portList_)
    seed = static_cast<Component*>(item)->propagate(seed);
  foreach(Item* item, userDefList_)
    seed = static_cast<Component*>(item)->propagate(seed);
  if(out_)
    seed = static_cast<Component*>(out_)->propagate(seed);

  // sub-circuits propagation
  foreach(Item* item, userDefList_) {
    QPointer<qsapecng::SchematicScene> rep =
      item->data(101).value< QPointer<qsapecng::SchematicScene> >();

    rep->assignNodes(seed);
  }
  passPrescribedPressureNodesTheirNodeIndices();
  detectComponentAt3DInterface();
  numberOfNodes_ = seed - 1;
  createInitialNodePressurePropertiesInBrowser(numberOfNodes_);
}

int SchematicScene::getNumberOfNodes() const
{
	if (numberOfNodes_ == -1)
	{
		throw crimson_bct_error_t::Error_NodesNotAssigned;
	}
	
	return numberOfNodes_;
}

void SchematicScene::createInitialNodePressurePropertiesInBrowser(const int numberOfNodes)
{
	// Ensure we're starting with a clean set of initial node pressure properties:
	properties_->removeSubProperty(typeRootMap_[AllNodesType]);
	QList<QtProperty*> existingNodalInitialPressureProperties = typeRootMap_[AllNodesType]->subProperties();
	foreach(QtProperty* subproperty, existingNodalInitialPressureProperties)
	{
		//\todo fix this so it doesnt forget pressure prescriptions the user has already set when you call this function
		typeRootMap_[AllNodesType]->removeSubProperty(subproperty);
	}

	// Add the (up-to-date for the present state of the circuit) nodal pressure properties:
	for (int node = 1; node <= numberOfNodes; node++)
	{
		QString propertyNameForThisPressureNode = "Initial Pressure at Node " + QString::number(node);
		QtProperty* nodalInitialPressureProperty = doublePropertyManager_->addProperty(propertyNameForThisPressureNode);
		doublePropertyManager_->setValue(nodalInitialPressureProperty, 0.0);
		doublePropertyManager_->setSingleStep(nodalInitialPressureProperty, 0.01);
		doublePropertyManager_->setDecimals(nodalInitialPressureProperty,13);

		typeRootMap_[AllNodesType]->addSubProperty(nodalInitialPressureProperty);
		properties_->addSubProperty(typeRootMap_[AllNodesType]);
	}
}

QtProperty* SchematicScene::getNodePropertyContainer() const
{
  return typeRootMap_[AllNodesType];
}

void SchematicScene::resetNodes()
{
	// Reset the node count to the nonsense value:
	numberOfNodes_ = -1;

  foreach(Item* item, groundList_)
    static_cast<Component*>(item)->invalidate();
  foreach(Item* item, threeDInterfaceNodeList_)
	  static_cast<Component*>(item)->invalidate();
  foreach(Item* item, wireList_)
    static_cast<Wire*>(item)->invalidate();
  foreach(Item* item, standardList_)
    static_cast<Component*>(item)->invalidate();
  foreach(Item* item, portList_)
    static_cast<Component*>(item)->invalidate();
  foreach(Item* item, userDefList_)
    static_cast<Component*>(item)->invalidate();
  if(out_)
    static_cast<Component*>(out_)->invalidate();

  // sub-circuits reset
  foreach(Item* item, userDefList_)
    item->data(101)
      .value< QPointer<qsapecng::SchematicScene> >()->resetNodes();
}


void SchematicScene::setGridVisible(bool visible)
{
  gridVisible_ = visible;
  update();

  foreach(QGraphicsView* view, views())
    view->resetCachedContent();
}


void SchematicScene::resetStatus()
{
  resetNodes();
  clearSelection();
  clearFocus();

  activeItem_ = NilItemType;
  delete item_;
  item_ = 0;

  foreach(QGraphicsView* view, views())
    view->unsetCursor();

  // userDefSessionRequested_ = false;
  // userDefMD5_ = QByteArray();
  // userDefSize_ = 0;

  wireSessionRequested_ = false;
  delete preWireItem_;
  delete postWireItem_;
  preWireItem_ = 0;
  postWireItem_ = 0;
  if(wireInProgress_) {
    wireInProgress_->setVisible(true);
    wireInProgress_ = 0;
  }
}


void SchematicScene::drawBackground(QPainter* painter, const QRectF& rect)
{
  if(gridVisible_) {
    painter->save();

    painter->fillRect(rect, Qt::white);
//     painter->setRenderHints(
//       QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter->setRenderHints(QPainter::SmoothPixmapTransform);
    painter->setPen(QPen(Qt::black, 0));
    painter->setBrush(Qt::NoBrush);

    int left = rect.x();
    if(left < 0) left -= (GridStep >> 1) - 1;
    else left += (GridStep >> 1);
    left -= left % GridStep;

    int top = rect.y();
    if(top < 0) top -= (GridStep >> 1) - 1;
    else top += (GridStep >> 1);
    top -= top % GridStep;

    int right = rect.x() + rect.width();
    int bottom = rect.y() + rect.height();

    for(int x = left; x < right; x += GridStep)
      for(int y = top; y < bottom; y += GridStep)
        painter->drawPoint(x, y);

    painter->restore();
  }
}


void SchematicScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  lastMousePressPos_ = event->scenePos();
  resetNodes();

  // if(hasActiveItem() || hasUserDefReq()) {
  if(hasActiveItem()) {
    switch(event->button())
    {
    case Qt::RightButton:
      item_->rotate();
      event->accept();
      return;
    case Qt::LeftButton:
    {
      QUndoCommand* addCommand = 0;
      
      // if(hasUserDefReq()) {
      //   addCommand = new AddUserDefItem(this,
      //     userDefMD5_,
      //     userDefSize_,
      //     userDefMap_.value(userDefMD5_),
      //     item_->pos());
      // } else {
          addCommand = new AddSupportedItem(this, activeItem_, item_->pos());
      // }

      undoRedoStack_->beginMacro(addCommand->text());
      undoRedoStack_->push(addCommand);

      uint rotFact = item_->angle() / 90;
      for(uint i = 0; i < rotFact; ++i) {
        RotateItems* rotateCommand = 0;

        // if(hasUserDefReq()) {
        //   rotateCommand = new RotateItems(static_cast<AddUserDefItem*>(addCommand)->item());
        // } else {
          rotateCommand = new RotateItems(static_cast<AddSupportedItem*>(addCommand)->item());
        // }

        undoRedoStack_->push(rotateCommand);
      }
      undoRedoStack_->endMacro();

      event->accept();
      return;
    }
    default:
      break;
    }
  }

  if(hasPendingWire()) {
    switch(event->button())
    {
    case Qt::RightButton:
    {
      if(hasValidFromPoint_) {
        horizontalFirst_ = !horizontalFirst_;
        setPrePostWireLine(event->scenePos());
      }
      event->accept();
      return;
    }
    case Qt::LeftButton:
    {
      if(!hasValidFromPoint_) {
        fromPoint_ = closestGridPoint(event->scenePos());
        preWireItem_->setVisible(true);
        hasValidFromPoint_ = true;
      } else {
        setPrePostWireLine(event->scenePos());
        addWires();
        hasValidFromPoint_ = false;
        preWireItem_->setVisible(false);
        postWireItem_->setVisible(false);
      }
      preWireItem_->setLine(QLineF(fromPoint_, fromPoint_));
      postWireItem_->setLine(QLineF(fromPoint_, fromPoint_));
      event->accept();
      return;
    }
    default:
      break;
    }
  }

  // if(!hasActiveItem() && !hasUserDefReq() && !hasPendingWire()
  if(!hasActiveItem() && !hasPendingWire()
      && selectedItems().isEmpty() && itemAt(event->scenePos(), QTransform())) {
    clearSelection();
	itemAt(event->scenePos(), QTransform())->setSelected(true);
  }

  QGraphicsScene::mousePressEvent(event);
}


void SchematicScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  // if(hasActiveItem() || hasUserDefReq()) {
  if(hasActiveItem()) {
    item_->setPos(closestGridPoint(event->scenePos()));
    event->accept();
    return;
  }

  if(hasPendingWire()) {
    if(hasValidFromPoint_)
      setPrePostWireLine(event->scenePos());
    event->accept();
    return;
  }

  QGraphicsScene::mouseMoveEvent(event);
}


void SchematicScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  // if(!hasActiveItem() && !hasUserDefReq() && !hasPendingWire()
	 //  && itemAt(event->scenePos(), QTransform())) {
  //   if(SchematicScene::itemType(itemAt(event->scenePos(), QTransform()))
  //       == SchematicScene::UserDefItemType)
  //   {
  //     QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
  //     QPointer<qsapecng::SchematicScene> rep =
  //       item->data(101).value< QPointer<qsapecng::SchematicScene> >();

  //     emit(showUserDef(*rep));
  //     event->accept();
  //   }
  // }

  // if(SchematicScene::itemType(itemAt(event->scenePos(), QTransform()))
  //     != SchematicScene::UserDefItemType)
    QGraphicsScene::mousePressEvent(event);
}


void SchematicScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  // if(!hasActiveItem() && !hasUserDefReq() && !hasPendingWire() && contextMenu_)
  if(!hasActiveItem() && !hasPendingWire() && contextMenu_)
    contextMenu_->exec(event->screenPos());
}


void SchematicScene::keyPressEvent(QKeyEvent* event)
{
  if(event->modifiers().testFlag(Qt::NoModifier)) {
    int key = event->key();

    switch(key)
    {
    case Qt::Key_Left:
      {
        moveSelectedItems(QPointF(-GridStep, 0));
        event->accept();
        return;
      }
    case Qt::Key_Up:
      {
        moveSelectedItems(QPointF(0, -GridStep));
        event->accept();
        return;
      }
    case Qt::Key_Right:
      {
        moveSelectedItems(QPointF(GridStep, 0));
        event->accept();
        return;
      }
    case Qt::Key_Down:
      {
        moveSelectedItems(QPointF(0, GridStep));
        event->accept();
        return;
      }
    case Qt::Key_Escape:
      {
        resetStatus();
        event->accept();
        return;
      }
    default:
      break;
    }

  }

  return QGraphicsScene::keyPressEvent(event);
}


bool SchematicScene::event(QEvent* event)
{
  // enter event
  if(event->type() == QEvent::Enter)
  {
    // if(hasActiveItem() || hasUserDefReq()) {
    if(hasActiveItem()) {
      if(item_)
        delete item_;

      item_ = SchematicScene::itemByType(activeItem_);
      
      // if(hasUserDefReq()) {
      //   static_cast<Component*>(item_)->setPath(userDefPath(userDefSize_));
      //   static_cast<Component*>(item_)->addNodes(userDefNodes(userDefSize_));
      // }
      
      addItem(item_);
      event->accept();
      return true;
    }
  }

  // leave event
  if(event->type() == QEvent::Leave)
  {
    // if(hasActiveItem() || hasUserDefReq()) {
    if(hasActiveItem()) {
      removeItem(item_);
      delete item_;
      item_ = 0;
      event->accept();
      return true;
    }
  }

  return QGraphicsScene::event(event);
}


void SchematicScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
  // if(hasActiveItem() || hasUserDefReq() || hasPendingWire()) {
  if(hasActiveItem() || hasPendingWire()) {
    event->ignore();
    return;
  }

  if(event->mimeData()->hasFormat("application/qsapecng-itemtype")) {
    SupportedItemType type = (SupportedItemType) event->mimeData()->data(
      "application/qsapecng-itemtype").toInt();

    item_ = itemByType(type);
    if(item_) {
      resetNodes();

      qreal xoffset = item_->boundingRect().width() / 2;
      qreal yoffset = item_->boundingRect().height() / 2;

      QPointF pos = closestGridPoint(
        event->scenePos() - QPointF(xoffset, yoffset)
      );

      addItem(item_);
      item_->setPos(pos);
      clearSelection();
      item_->setSelected(true);
      event->acceptProposedAction();
    }
  } else {
    event->ignore();
  }
}


void SchematicScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
  if(item_) {
    qreal xoffset = item_->boundingRect().width() / 2;
    qreal yoffset = item_->boundingRect().height() / 2;

    QPointF pos = closestGridPoint(
      event->scenePos() - QPointF(xoffset, yoffset)
    );
    item_->setPos(pos);
    event->acceptProposedAction();
  } else {
    event->ignore();
  }
}


void SchematicScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
{
  if(item_) {
    removeItem(item_);
    delete item_;
    item_ = 0;
    event->acceptProposedAction();
  } else {
    event->ignore();
  }
}


void SchematicScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
  if(item_) {
    SupportedItemType type = (SupportedItemType) item_->data(0).toInt();
    qreal xoffset = item_->boundingRect().width() / 2;
    qreal yoffset = item_->boundingRect().height() / 2;

    removeItem(item_);
    delete item_;
    item_ = 0;

    AddSupportedItem* command = new AddSupportedItem(this, type,
      event->scenePos() - QPointF(xoffset, yoffset));
    undoRedoStack_->push(command);

    event->acceptProposedAction();
  } else {
    event->ignore();
  }
}


void SchematicScene::init()
{
  out_ = 0;
  contextMenu_ = 0;
  validNode_ = false;
  gridVisible_ = true;
  undoRedoStack_ = new QUndoStack;
  lastMousePressPos_ = QPointF(0, 0);
  setItemIndexMethod(NoIndex);
  activeItem_ = NilItemType;
  item_ = 0;

  // userDefSessionRequested_ = false;
  // userDefMD5_ = QByteArray();
  // userDefSize_ = 0;

  wireSessionRequested_ = false;
  wireInProgress_ = 0;
  preWireItem_ = 0;
  postWireItem_ = 0;
  wirePen_ = QPen(
      Qt::SolidPattern,
      1,
      Qt::DashLine,
      Qt::RoundCap,
      Qt::RoundJoin
    );

  connect(undoRedoStack_, SIGNAL(indexChanged(int)),
    this, SLOT(resetNodes()));
  numberOfNodes_ = -1; // Initialse to a nonsense value
}


void SchematicScene::setupProperties()
{
  groupManager_ = new QtGroupPropertyManager(this);

  boolManager_ = new QtBoolPropertyManager(this);
  stringManager_ = new QtStringPropertyManager(this);
  managerForAllDropdownBoxProperties_ = new QtVariantPropertyManager(this);
  doublePropertyManager_ = new QtDoublePropertyManager(this);

  connect(boolManager_, SIGNAL(propertyChanged(QtProperty*)),
    this, SIGNAL(propertyChanged()));
  connect(stringManager_, SIGNAL(propertyChanged(QtProperty*)),
    this, SIGNAL(propertyChanged()));
  connect(managerForAllDropdownBoxProperties_, SIGNAL(propertyChanged(QtProperty*)),
	  this, SIGNAL(propertyChanged()));
  connect(doublePropertyManager_, SIGNAL(propertyChanged(QtProperty*)),
	  this, SIGNAL(propertyChanged()));

  lineEditFactory_ = new QtLineEditFactory(this);
  lineEditFactory_->addPropertyManager(stringManager_);
  
  checkBoxFactory_ = new QtCheckBoxFactory(this);
  checkBoxFactory_->addPropertyManager(boolManager_);
  
  variantFactory_ = new QtVariantEditorFactory(this);
  variantFactory_->addPropertyManager(managerForAllDropdownBoxProperties_);

  doubleSpinboxFactory_ = new QtDoubleSpinBoxFactory(this);
  doubleSpinboxFactory_->addPropertyManager(doublePropertyManager_);

  properties_ = groupManager_->addProperty(tr("Circuit"));

  typeRootMap_[AllNodesType] = groupManager_->addProperty(tr("Initial Node Pressures"));

  itemCntMap_[CapacitorItemType] = 0;
  typeRootMap_[CapacitorItemType] =
    groupManager_->addProperty(itemNameByType(CapacitorItemType));
//   properties_->addSubProperty(typeRootMap_[CapacitorItemType]);

  itemCntMap_[DiodeItemType] = 0;
  typeRootMap_[DiodeItemType] =
  groupManager_->addProperty(itemNameByType(DiodeItemType));
  
  itemCntMap_[VolumeTrackingChamberItemType] = 0;
  typeRootMap_[VolumeTrackingChamberItemType] =
	  groupManager_->addProperty(itemNameByType(VolumeTrackingChamberItemType));

  itemCntMap_[CCCSItemType] = 0;
  typeRootMap_[CCCSItemType] =
    groupManager_->addProperty(itemNameByType(CCCSItemType));
//   properties_->addSubProperty(typeRootMap_[CCCSItemType]);

  itemCntMap_[CCVSItemType] = 0;
  typeRootMap_[CCVSItemType] =
    groupManager_->addProperty(itemNameByType(CCVSItemType));
//   properties_->addSubProperty(typeRootMap_[CCVSItemType]);

  itemCntMap_[ConductanceItemType] = 0;
  typeRootMap_[ConductanceItemType] =
    groupManager_->addProperty(itemNameByType(ConductanceItemType));
//   properties_->addSubProperty(typeRootMap_[ConductanceItemType]);

  itemCntMap_[CurrentSourceItemType] = 0;
  typeRootMap_[CurrentSourceItemType] =
    groupManager_->addProperty(itemNameByType(CurrentSourceItemType));
//   properties_->addSubProperty(typeRootMap_[CurrentSourceItemType]);

  itemCntMap_[InductorItemType] = 0;
  typeRootMap_[InductorItemType] =
    groupManager_->addProperty(itemNameByType(InductorItemType));
//   properties_->addSubProperty(typeRootMap_[InductorItemType]);

  itemCntMap_[OpAmplItemType] = 0;
  typeRootMap_[OpAmplItemType] =
    groupManager_->addProperty(itemNameByType(OpAmplItemType));
//   properties_->addSubProperty(typeRootMap_[OpAmplItemType]);

  itemCntMap_[ResistorItemType] = 0;
  typeRootMap_[ResistorItemType] =
    groupManager_->addProperty(itemNameByType(ResistorItemType));
//   properties_->addSubProperty(typeRootMap_[ResistorItemType]);

  itemCntMap_[threeDInterfaceNodeType] = 0;
  typeRootMap_[threeDInterfaceNodeType] =
	  groupManager_->addProperty(itemNameByType(threeDInterfaceNodeType));

  itemCntMap_[prescribedPressureNodeType] = 0;
  typeRootMap_[prescribedPressureNodeType] = groupManager_->addProperty(itemNameByType(prescribedPressureNodeType));

  itemCntMap_[VCCSItemType] = 0;
  typeRootMap_[VCCSItemType] =
    groupManager_->addProperty(itemNameByType(VCCSItemType));
//   properties_->addSubProperty(typeRootMap_[VCCSItemType]);

  itemCntMap_[VCVSItemType] = 0;
  typeRootMap_[VCVSItemType] =
    groupManager_->addProperty(itemNameByType(VCVSItemType));
//   properties_->addSubProperty(typeRootMap_[VCVSItemType]);

  itemCntMap_[VoltageSourceItemType] = 0;
  typeRootMap_[VoltageSourceItemType] =
    groupManager_->addProperty(itemNameByType(VoltageSourceItemType));
//   properties_->addSubProperty(typeRootMap_[VoltageSourceItemType]);

  itemCntMap_[TransformerItemType] = 0;
  typeRootMap_[TransformerItemType] =
    groupManager_->addProperty(itemNameByType(TransformerItemType));
//   properties_->addSubProperty(typeRootMap_[TransformerItemType]);

  itemCntMap_[MutualInductanceItemType] = 0;
  typeRootMap_[MutualInductanceItemType] =
    groupManager_->addProperty(itemNameByType(MutualInductanceItemType));
//   properties_->addSubProperty(typeRootMap_[MutualInductanceItemType]);

  // itemCntMap_[UserDefItemType] = 0;
  // typeRootMap_[UserDefItemType] =
  //   groupManager_->addProperty(itemNameByType(UserDefItemType));
//   properties_->addSubProperty(typeRootMap_[UserDefItemType]);
}


void SchematicScene::setPrePostWireLine(QPointF toPoint)
{
  QPointF realToPoint = closestGridPoint(toPoint);

  if(fromPoint_.x() == realToPoint.x() || fromPoint_.y() == realToPoint.y()) {
    preWireItem_->setLine(QLineF(fromPoint_, realToPoint));
    postWireItem_->setVisible(false);
  } else {
    QPointF joint(
        (horizontalFirst_ ? realToPoint.x() : fromPoint_.x()),
        (horizontalFirst_ ? fromPoint_.y() : realToPoint.y())
      );

    preWireItem_->setLine(QLineF(fromPoint_, joint));
    postWireItem_->setLine(QLineF(joint, realToPoint));
    postWireItem_->setVisible(true);
  }
}


void SchematicScene::addWires()
{
  QPointF pos;

  if(wireInProgress_) {
    undoRedoStack_->beginMacro(tr("Modify wire"));
    undoRedoStack_->push(new RemoveItems(this, wireInProgress_));
    wireInProgress_->setVisible(true);
  }

  if(preWireItem_->line().y1() == preWireItem_->line().y2())
  {
    if(preWireItem_->line().x1() > preWireItem_->line().x2())
      pos = preWireItem_->line().p2();
    else
      pos = preWireItem_->line().p1();
  } else {
    if(preWireItem_->line().y1() > preWireItem_->line().y2())
      pos = preWireItem_->line().p2();
    else
      pos = preWireItem_->line().p1();
  }

  AddWire* preWireCommand = new AddWire(this, pos,
    preWireItem_->line(), connectedWire_);
  undoRedoStack_->push(preWireCommand);

  if(postWireItem_->isVisible()) {
    if(postWireItem_->line().y1() == postWireItem_->line().y2())
    {
      if(postWireItem_->line().x1() > postWireItem_->line().x2())
        pos = postWireItem_->line().p2();
      else
        pos = postWireItem_->line().p1();
    } else {
      if(postWireItem_->line().y1() > postWireItem_->line().y2())
        pos = postWireItem_->line().p2();
      else
        pos = postWireItem_->line().p1();
    }

    AddWire* postWireCommand = new AddWire(this, pos,
      postWireItem_->line(), connectedWire_);
    undoRedoStack_->push(postWireCommand);
  }

  if(wireInProgress_) {
    undoRedoStack_->endMacro();
    wireInProgress_ = 0;
  }
}


}
