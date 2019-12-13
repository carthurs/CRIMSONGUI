#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateAnd.h>
#include <NodePredicateDerivation.h>
#include <NodePredicateNone.h>

#include <ISolverStudyData.h>
#include <SolverSetupNodeTypes.h>

#include "SolverStudyUIDsTableModel.h"

namespace crimson
{

SolverStudyUIDsTableModel::SolverStudyUIDsTableModel(HierarchyManager::NodeType nodeType, mitk::DataNode* solverStudyNode,
                                                     QObject* parent)
    : QmitkDataStorageTableModel(nullptr, nullptr, parent)
    , _nodeType(std::move(nodeType))
{
    Expects(nodeType == SolverSetupNodeTypes::BoundaryConditionSet() || nodeType == SolverSetupNodeTypes::Material());

    setSolverStudyNode(solverStudyNode);
}

void SolverStudyUIDsTableModel::setSolverStudyNode(mitk::DataNode* solverStudyNode)
{
    if (_solverStudyNode == solverStudyNode) {
        return;
    }
    _solverStudyNode = solverStudyNode;

    auto hm = HierarchyManager::getInstance();

    if (solverStudyNode) {
        auto solverRoot = hm->getAncestor(solverStudyNode, SolverSetupNodeTypes::SolverRoot());

        SetPredicate(mitk::NodePredicateAnd::New(NodePredicateDerivation::New(solverRoot, true, hm->getDataStorage()),
                                                 hm->getPredicate(_nodeType)));

        SetDataStorage(hm->getDataStorage());

        _solverStudy = static_cast<ISolverStudyData*>(solverStudyNode->GetData());

        // TODO: connect to modified event of solver study data
    } else {
        SetPredicate(NodePredicateNone::New());
        _solverStudy = nullptr;
        // TODO: release connection
    }
}

std::vector<mitk::DataNode*> SolverStudyUIDsTableModel::getCheckedNodes() const
{
    std::vector<mitk::DataNode*> checkedNodes;

    for (int row = 0; row < rowCount(QModelIndex{}); ++row) {
        if (data(index(row, 1), Qt::CheckStateRole).toInt() == QVariant{Qt::Checked}) {
            checkedNodes.push_back(GetNode(index(row, 0)).GetPointer());
        }
    }

    return checkedNodes;
}

QVariant SolverStudyUIDsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section != 1) {
        return QmitkDataStorageTableModel::headerData(section, orientation, role);
    }

    return {"Used"};
}

Qt::ItemFlags SolverStudyUIDsTableModel::flags(const QModelIndex& index) const
{
    if (index.column() != 1) {
        return QmitkDataStorageTableModel::flags(index);
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
}

int SolverStudyUIDsTableModel::columnCount(const QModelIndex& parent) const { return 2; }

QVariant SolverStudyUIDsTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant{};
    }

    if (index.column() != 1) {
        return QmitkDataStorageTableModel::data(index, role);
    }

    if (role == Qt::CheckStateRole) {
        auto nodeUID = std::string{};
        GetNode(index)->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, nodeUID);

        auto uids = _nodeType == SolverSetupNodeTypes::BoundaryConditionSet() ? _solverStudy->getBoundaryConditionSetNodeUIDs()
                                                                              : _solverStudy->getMaterialNodeUIDs();
        return std::find_if(uids.begin(), uids.end(), [nodeUID](gsl::cstring_span<> uid) {
                   return nodeUID.compare(0, uid.size(), uid.data(), uid.size()) == 0;
               }) == uids.end() ? Qt::Unchecked : Qt::Checked;
    }

    return QVariant{};
}

bool SolverStudyUIDsTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (index.column() != 1) {
        return QmitkDataStorageTableModel::setData(index, value, role);
    }

    if (role == Qt::CheckStateRole) {
        auto nodeUID = std::string{};
        GetNode(index)->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, nodeUID);

        auto uids = _nodeType == SolverSetupNodeTypes::BoundaryConditionSet() ? _solverStudy->getBoundaryConditionSetNodeUIDs()
                                                                              : _solverStudy->getMaterialNodeUIDs();
        auto newUIDs = std::vector<gsl::cstring_span<>>{uids.begin(), uids.end()};
        if (value == Qt::Checked) {
            newUIDs.push_back(nodeUID);
        } else {
            newUIDs.erase(std::remove_if(newUIDs.begin(), newUIDs.end(), [nodeUID](gsl::cstring_span<> uid) {
                return nodeUID.compare(0, uid.size(), uid.data(), uid.size()) == 0;
            }));
        }

        if (_nodeType == SolverSetupNodeTypes::BoundaryConditionSet()) {
            _solverStudy->setBoundaryConditionSetNodeUIDs(newUIDs);
        } else {
            _solverStudy->setMaterialNodeUIDs(newUIDs);
        }
        return true;
    }

    return false;
}
}
