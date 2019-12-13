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


#include "gui/sidebarmodel.h"

#include "gui/editor/schematicscene.h"

#include <QtCore/QMap>
#include <QtCore/QMimeData>
#include <QPainter>




namespace qsapecng
{


SideBarModel::SideBarModel(QObject* parent)
  : QAbstractListModel(parent)
{
  setupSideBar();
}


int SideBarModel::rowCount(const QModelIndex& parent) const
{
  return componentList_.size();
}


QVariant SideBarModel::data(const QModelIndex& index, int role) const
{
  if(index.isValid())
    return (componentList_.at(index.row()))[role];

  return QVariant();
}


QVariant SideBarModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
  ) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();

  return tr("Components");
}


Qt::ItemFlags SideBarModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

  if(index.isValid())
    return Qt::ItemIsDragEnabled | defaultFlags;
  else
    return defaultFlags;
}


Qt::DropActions SideBarModel::supportedDropActions() const
{
  return Qt::IgnoreAction;
}


QStringList SideBarModel::mimeTypes() const
{
  QStringList types;
  types << "application/qsapecng-itemtype";

  return types;
}


QMimeData* SideBarModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData;

  QModelIndex index;
  if(!indexes.isEmpty())
    index = indexes.first();

  if(index.isValid()) {
    int type = (componentList_.at(index.row())[Qt::UserRole]).toInt();
    QByteArray encodedData = QByteArray::number(type);
    mimeData->setData("application/qsapecng-itemtype", encodedData);
  }

  return mimeData;
}


void SideBarModel::setupSideBar()
{
	int paddingWidth = 128;
	int paddingHeight = 64;
	{
		QMap<int, QVariant> threeDInterfaceItemData;

		threeDInterfaceItemData[Qt::DisplayRole] = tr("3D Interface Node");
		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/threeDNode.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		threeDInterfaceItemData[Qt::DecorationRole] = imageBackgroundPadding;
		threeDInterfaceItemData[Qt::ToolTipRole] = tr("3D Interface Item");
		threeDInterfaceItemData[Qt::UserRole] = SchematicScene::threeDInterfaceNodeType;
		componentList_.append(threeDInterfaceItemData);
	}

	{
		QMap<int, QVariant> resistorItemData;
		resistorItemData[Qt::DisplayRole] = tr("Resistor");

		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/resistor.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		resistorItemData[Qt::DecorationRole] = imageBackgroundPadding;
		resistorItemData[Qt::ToolTipRole] = tr("Resistor Item");
		resistorItemData[Qt::UserRole] = SchematicScene::ResistorItemType;
		componentList_.append(resistorItemData);
	}

  //QMap<int, QVariant> conductanceItemData;
  //conductanceItemData[Qt::DisplayRole] = tr("Conductance");
  //conductanceItemData[Qt::DecorationRole] =
  //  QPixmap(":/images/symbols/resistor.png");
  //conductanceItemData[Qt::ToolTipRole] = tr("Conductance Item");
  //conductanceItemData[Qt::UserRole] = SchematicScene::ConductanceItemType;
  //componentList_.append(conductanceItemData);
	{
		QMap<int, QVariant> inductorItemData;
		inductorItemData[Qt::DisplayRole] = tr("Inductor");

		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/inductor.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		inductorItemData[Qt::DecorationRole] = imageBackgroundPadding;
		inductorItemData[Qt::ToolTipRole] = tr("Inductor Item");
		inductorItemData[Qt::UserRole] = SchematicScene::InductorItemType;
		componentList_.append(inductorItemData);
	}

	{
		QMap<int, QVariant> capacitorItemData;
		capacitorItemData[Qt::DisplayRole] = tr("Capacitor");

		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/capacitor.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		capacitorItemData[Qt::DecorationRole] = imageBackgroundPadding;
		capacitorItemData[Qt::ToolTipRole] = tr("Capacitor Item");
		capacitorItemData[Qt::UserRole] = SchematicScene::CapacitorItemType;
		componentList_.append(capacitorItemData);
	}

	{
		QMap<int, QVariant> diodeItemData;
		diodeItemData[Qt::DisplayRole] = tr("Diode");

		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/diode.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		diodeItemData[Qt::DecorationRole] = imageBackgroundPadding;
		diodeItemData[Qt::ToolTipRole] = tr("Diode Item");
		diodeItemData[Qt::UserRole] = SchematicScene::DiodeItemType;
		componentList_.append(diodeItemData);
	}

	{
		QMap<int, QVariant> volumeTrackingChamberItemData;
		volumeTrackingChamberItemData[Qt::DisplayRole] = tr("Volume-Tracking Pressure Chamber");

		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/volumeTrackingPressureChamber.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		volumeTrackingChamberItemData[Qt::DecorationRole] = imageBackgroundPadding;
		volumeTrackingChamberItemData[Qt::ToolTipRole] = tr("Volume-Tracking Pressure Chamber Item");
		volumeTrackingChamberItemData[Qt::UserRole] = SchematicScene::VolumeTrackingChamberItemType;
		componentList_.append(volumeTrackingChamberItemData);
	}

	{
		QMap<int, QVariant> prescribedPressureNodeItemData;
		prescribedPressureNodeItemData[Qt::DisplayRole] = tr("Prescribed Pressure Node");

		QPixmap imageBackgroundPadding(paddingWidth, paddingHeight);
		imageBackgroundPadding.fill(); //default fill with white
		QPainter painter(&imageBackgroundPadding);
		QPixmap image = QPixmap(":/images/symbols/prescribedPressureNode.png");
		int topLeftCornerX = paddingWidth/2 - floor(image.width() / 2);
		int topLeftCornerY = paddingHeight/2 - floor(image.height() / 2);
		painter.drawPixmap(topLeftCornerX, topLeftCornerY, image);

		prescribedPressureNodeItemData[Qt::DecorationRole] = imageBackgroundPadding;
		prescribedPressureNodeItemData[Qt::ToolTipRole] = tr("Prescribed Pressure Node");
		prescribedPressureNodeItemData[Qt::UserRole] = SchematicScene::prescribedPressureNodeType;
		componentList_.append(prescribedPressureNodeItemData);
	}
  //QMap<int, QVariant> voltageSourceItemData;
  //voltageSourceItemData[Qt::DisplayRole] = tr("Voltage Source");
  //voltageSourceItemData[Qt::DecorationRole] =
  //  QPixmap(":/images/symbols/voltage.png");
  //voltageSourceItemData[Qt::ToolTipRole] = tr("Voltage Source Item");
  //voltageSourceItemData[Qt::UserRole] = SchematicScene::VoltageSourceItemType;
  //componentList_.append(voltageSourceItemData);

  //QMap<int, QVariant> currentSourceItemData;
  //currentSourceItemData[Qt::DisplayRole] = tr("Current Source");
  //currentSourceItemData[Qt::DecorationRole] =
  //  QPixmap(":/images/symbols/current.png");
  //currentSourceItemData[Qt::ToolTipRole] = tr("Current Source Item");
  //currentSourceItemData[Qt::UserRole] = SchematicScene::CurrentSourceItemType;
  //componentList_.append(currentSourceItemData);

  //QMap<int, QVariant> vcvsItemData;
  //vcvsItemData[Qt::DisplayRole] = tr("VCVS");
  //vcvsItemData[Qt::DecorationRole] = QPixmap(":/images/symbols/vcvs.png");
  //vcvsItemData[Qt::ToolTipRole] = tr("VCVS Item");
  //vcvsItemData[Qt::UserRole] = SchematicScene::VCVSItemType;
  //componentList_.append(vcvsItemData);

  //QMap<int, QVariant> ccvsItemData;
  //ccvsItemData[Qt::DisplayRole] = tr("CCVS");
  //ccvsItemData[Qt::DecorationRole] = QPixmap(":/images/symbols/ccvs.png");
  //ccvsItemData[Qt::ToolTipRole] = tr("CCVS Item");
  //ccvsItemData[Qt::UserRole] = SchematicScene::CCVSItemType;
  //componentList_.append(ccvsItemData);

  //QMap<int, QVariant> vccsItemData;
  //vccsItemData[Qt::DisplayRole] = tr("VCCS");
  //vccsItemData[Qt::DecorationRole] = QPixmap(":/images/symbols/vccs.png");
  //vccsItemData[Qt::ToolTipRole] = tr("VCCS Item");
  //vccsItemData[Qt::UserRole] = SchematicScene::VCCSItemType;
  //componentList_.append(vccsItemData);

  //QMap<int, QVariant> cccsItemData;
  //cccsItemData[Qt::DisplayRole] = tr("CCCS");
  //cccsItemData[Qt::DecorationRole] = QPixmap(":/images/symbols/cccs.png");
  //cccsItemData[Qt::ToolTipRole] = tr("CCCS Item");
  //cccsItemData[Qt::UserRole] = SchematicScene::CCCSItemType;
  //componentList_.append(cccsItemData);

  //QMap<int, QVariant> opAmplItemData;
  //opAmplItemData[Qt::DisplayRole] = tr("Operational Amplifier");
  //opAmplItemData[Qt::DecorationRole] = QPixmap(":/images/symbols/opampl.png");
  //opAmplItemData[Qt::ToolTipRole] = tr("Operational Amplifier Item");
  //opAmplItemData[Qt::UserRole] = SchematicScene::OpAmplItemType;
  //componentList_.append(opAmplItemData);

  //QMap<int, QVariant> transformerItemData;
  //transformerItemData[Qt::DisplayRole] = tr("Ideal Transformer");
  //transformerItemData[Qt::DecorationRole] =
  //  QPixmap(":/images/symbols/transformer.png");
  //transformerItemData[Qt::ToolTipRole] = tr("Ideal Transformer Item");
  //transformerItemData[Qt::UserRole] = SchematicScene::TransformerItemType;
  //componentList_.append(transformerItemData);

  //QMap<int, QVariant> mutualInductanceItemData;
  //mutualInductanceItemData[Qt::DisplayRole] = tr("Mutual Inductance");
  //mutualInductanceItemData[Qt::DecorationRole] =
  //  QPixmap(":/images/symbols/mutualinductance.png");
  //mutualInductanceItemData[Qt::ToolTipRole] = tr("Mutual Inductance Item");
  //mutualInductanceItemData[Qt::UserRole] = SchematicScene::MutualInductanceItemType;
  //componentList_.append(mutualInductanceItemData);
}


}
