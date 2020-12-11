#pragma once

#include <QmitkDataStorageTableModel.h>
#include <HierarchyManager.h>

#include "ImmutableRanges.h"

namespace crimson
{

class ISolverStudyData;

/*!
 * \brief   A table model responsible for displaying and setting selected a collection of node
 *  uids (bc sets or materials) for a solver study from the same solver root.
 */
class SolverStudyUIDsTableModel : public QmitkDataStorageTableModel
{
    Q_OBJECT
public:
    SolverStudyUIDsTableModel(HierarchyManager::NodeType nodeType, mitk::DataNode* solverStudyNode = nullptr, QObject* parent = nullptr);

    void setSolverStudyNode(mitk::DataNode* solverStudyNode);
    std::vector<mitk::DataNode*> getCheckedNodes() const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;


private:
    mitk::DataNode* _solverStudyNode = nullptr;
    ISolverStudyData* _solverStudy = nullptr;
    HierarchyManager::NodeType _nodeType;

	ImmutableValueRange<gsl::cstring_span<>> _getOwnedUIDs(const int columnIndex) const;
};
}