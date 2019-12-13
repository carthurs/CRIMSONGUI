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


#ifndef SCHEMATICSCENE_H
#define SCHEMATICSCENE_H


#include <QGraphicsScene>

#include <QtCore/QList>
#include <QtCore/QRect>
#include <QtCore/QHash>
#include <QtCore/QPointF>
#include <QtCore/QVariant>
#include <QtCore/QMetaType>
#include <QtCore/QPointer>

#include <QtProperty>
#include <QtVariantPropertyManager>
#include <QtDoublePropertyManager>
#include <QtDoubleSpinBoxFactory>
#include "QtStringPropertyManager"

#include <memory>


class QMenu;
class QEvent;
class QPainter;
class QKeyEvent;
class QUndoStack;
class QUndoCommand;
class QGraphicsItem;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneContextMenuEvent;

class QtBoolPropertyManager;
class QtGroupPropertyManager;
class QtStringPropertyManager;

class QtLineEditFactory;
class QtCheckBoxFactory;

class QtAbstractPropertyBrowser;
class QtProperty;


namespace qsapecng
{


class Component;
class Item;
class Wire;

class SchematicScene: public QGraphicsScene
{

  Q_OBJECT

public:
  enum { Ground = 0 };
  enum { GridStep = 10 };

  enum SupportedItemType {
    NilItemType = 0,
    
    // UserDefItemType,

	threeDInterfaceNodeType,
	prescribedPressureNodeType,
	AllNodesType,

	DiodeItemType,
	VolumeTrackingChamberItemType,
    CapacitorItemType,
    CCCSItemType,
    CCVSItemType,
    ConductanceItemType,
    CurrentSourceItemType,
    InductorItemType,
    OpAmplItemType,
    ResistorItemType,
    VCCSItemType,
    VCVSItemType,
    VoltageSourceItemType,
    TransformerItemType,
    MutualInductanceItemType,

    // VoltmeterItemType,
    // AmmeterItemType,

    GroundItemType,
    // PortItemType,
    // OutItemType,
    WireItemType,

    LabelItemType,
  };

  static QPainterPath nilPath();

  static QPainterPath groundPath();
  static QPainterPath portPath();
  static QPainterPath outPath();
  static QPainterPath voltmeterPath();
  static QPainterPath ammeterPath();

  static QPainterPath volumeTrackingPressureChamberPath();
  static QPainterPath capacitorPath();
  static QPainterPath diodePath();
  static QPainterPath cccsPath();
  static QPainterPath ccvsPath();
  static QPainterPath conductancePath();
  static QPainterPath currentSourcePath();
  static QPainterPath inductorPath();
  static QPainterPath opAmplPath();
  static QPainterPath resistorPath();
  static QPainterPath threeDInterfaceNodePath();
  static QPainterPath prescribedPressureNodePath();
  static QPainterPath vccsPath();
  static QPainterPath vcvsPath();
  static QPainterPath voltageSourcePath();
  static QPainterPath transformerPath();
  static QPainterPath mutualInductancePath();

  static QtProperty* itemProperties(QGraphicsItem* item);
  static SchematicScene::SupportedItemType itemType(QGraphicsItem* item);
  static Item* itemByType(SupportedItemType type);
  static QString itemNameByType(SchematicScene::SupportedItemType type);
  static QChar itemIdByType(SchematicScene::SupportedItemType type);

  static QPainterPath userDefPath(uint ports);
  static QList<QPointF> userDefNodes(uint ports);

public:
  SchematicScene(QObject* parent = 0);
  ~SchematicScene();

  QPointF closestGridPoint(const QPointF& pos) const;
  inline bool gridVisible() const { return gridVisible_; }
  inline bool isGridPoint(QPointF point) const
    { return (point == closestGridPoint(point)); }

  void setActiveItem(SupportedItemType item);
  inline SupportedItemType activeItem() const { return activeItem_; }
  QList<Item*> activeItems() const;
  QList<Item*> getStandardList() const;
  QList<Component*> getAllPrescribedPressureNodes() const;

  QByteArray registerUserDef(const SchematicScene& scene);
  std::string queryUserDef(QByteArray md5);
  // void setUserDefRequest();

  void addItems(QList<QGraphicsItem*> items);
  void addItems(QGraphicsItem* item);
  void addSupportedItem(QGraphicsItem* gItem, bool init = true);
  void removeSupportedItem(QGraphicsItem* gItem);
  void clearSupportedItem(QGraphicsItem* gItem);

  void setWireSessionRequest(bool connectedWire);
  void joinWires(Wire* w1, Wire* w2);
  void modifyWire(Wire* wire);

  void createLabel();

  QtProperty* getNodePropertyContainer() const;
  int getNumberOfNodes() const;

  void moveSelectedItems(QPointF pos);
  void cutSelectedItems();
  void copySelectedItems();
  void pasteItems();
  void rotateSelectedItems();
  void mirrorSelectedItems();
  void binSelectedItems();
  void bringToFrontSelectedItem();
  void sendToBackSelectedItem();
  void clearSchematicScene();

  inline void setContextMenu(QMenu* menu) { contextMenu_ = menu; }
  inline QMenu* contextMenu() const { return contextMenu_; }

  inline QUndoStack* undoRedoStack() const { return undoRedoStack_; }

  void initializeBrowser(QtAbstractPropertyBrowser* browser);
  inline QtProperty* properties() const { return properties_; }

  QVector<int> ports();
  inline int size() { return portList_.size(); }
  void assignNodes(int seed = SchematicScene::Ground + 1);

public slots:
  void resetNodes();
  void setGridVisible(bool visible = true);
  void resetStatus();

signals:
  void showUserDef(SchematicScene& scene);
  void propertyChanged();

protected:
  void drawBackground(QPainter* painter, const QRectF& rect);
  void mousePressEvent(QGraphicsSceneMouseEvent* event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
  void keyPressEvent(QKeyEvent* event);
  bool event(QEvent* event);

  void dragEnterEvent(QGraphicsSceneDragDropEvent* event);
  void dragMoveEvent(QGraphicsSceneDragDropEvent* event);
  void dragLeaveEvent(QGraphicsSceneDragDropEvent* event);
  void dropEvent(QGraphicsSceneDragDropEvent* event);

private:
  void init();
  void setupProperties();

  inline bool hasActiveItem() const { return (activeItem_ != NilItemType); }
  // inline bool hasUserDefReq() const { return userDefSessionRequested_; }
  inline bool hasPendingWire() const { return wireSessionRequested_; }
  void setPrePostWireLine(QPointF toPoint);
  void addWires();
  void passPrescribedPressureNodesTheirNodeIndices();
  void detectComponentAt3DInterface();
  void createInitialNodePressurePropertiesInBrowser(const int numberOfNodes);

  // The name of this function only approximates its functionality; it's a slightly arbitrary grouping of code coming from a refactor of a terribad case statement
  void setupItemLabel(Item* const item, QtProperty* const name);
  void setupComponentControlSpecification(QtVariantProperty*& prop, QtProperty*& customDataFileName);
  void setupMutualInductanceProperties(QtProperty* const name);
  void setLabelAndAddComponentToHierarchy(Item* const item, QtProperty* const name, SupportedItemType const type);

  QtProperty* setupCrimsonBctComponentCommon(Item* const item, SupportedItemType const type, const bool initialise);
  void addInitialVolumeParameterSetter(QtProperty* const name);
private:
  bool gridVisible_;
  QMenu* contextMenu_;
  QPointF lastMousePressPos_;
  bool validNode_;
  
  SupportedItemType activeItem_;
  Item* item_;

  bool wireSessionRequested_;
  bool horizontalFirst_;
  bool hasValidFromPoint_;
  bool connectedWire_;
  QPointF fromPoint_;
  QGraphicsLineItem* preWireItem_;
  QGraphicsLineItem* postWireItem_;
  Wire* wireInProgress_;
  QPen wirePen_;

  Item* out_;
  QList<Item*> outList_;
  QList<Item*> portList_;
  QList<Item*> groundList_;
  QList<Item*> standardList_;
  QList<Item*> wireList_;
  QList<Item*> labelList_;
  QList<Item*> userDefList_;
  QList<Item*> threeDInterfaceNodeList_;
  QList<Item*> prescribedPressureNodesList_;
  
  // int userDefSize_;
  // QByteArray userDefMD5_;
  QHash<QByteArray, std::string> userDefMap_;
  // bool userDefSessionRequested_;

  QUndoStack* undoRedoStack_;

  QtAbstractPropertyBrowser* browser_;
  QtBoolPropertyManager* boolManager_;
  QtGroupPropertyManager* groupManager_;
  QtStringPropertyManager* stringManager_;
  //QList<QSharedPointer<QtStringPropertyManager_Disableable>> disableableStringManagersList_;
  QtDoublePropertyManager* doublePropertyManager_; // Not sure why we're fudging a QtStringPropertyManager to hold doubles, when this exists. \todo transition to QtDoublePropertyManager wherever possible.
  QtVariantPropertyManager* managerForAllDropdownBoxProperties_;
  QtLineEditFactory* lineEditFactory_;
  QtCheckBoxFactory* checkBoxFactory_;
  QtDoubleSpinBoxFactory* doubleSpinboxFactory_;
  QtVariantEditorFactory* variantFactory_;

  QtProperty* properties_;
  QHash<SupportedItemType, unsigned int> itemCntMap_;
  QHash<SupportedItemType, QtProperty*> typeRootMap_;
  int numberOfNodes_;

};


}

Q_DECLARE_METATYPE(QPointer<qsapecng::SchematicScene>);
Q_DECLARE_METATYPE(QtProperty*);


#endif // SCHEMATICSCENE_H
