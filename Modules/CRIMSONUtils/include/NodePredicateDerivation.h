#pragma once

#include <mitkWeakPointer.h>
#include <mitkDataNode.h>
#include <mitkDataStorage.h>
#include <mitkNodePredicateBase.h>

#include "CRIMSONUtilsExports.h"

namespace crimson
{
/*! \brief   A node predicate identifying derivations of a particular mitk::DataNode. */
class CRIMSONUtils_EXPORT NodePredicateDerivation : public mitk::NodePredicateBase
{
public:
    mitkClassMacro(NodePredicateDerivation, mitk::NodePredicateBase);
    mitkNewMacro3Param(NodePredicateDerivation, mitk::DataNode*, bool, mitk::DataStorage*);

    bool CheckNode(const mitk::DataNode* node) const override;

protected:
    NodePredicateDerivation(mitk::DataNode* n, bool allderiv, mitk::DataStorage* ds);

    mitk::WeakPointer<mitk::DataNode> m_ParentNode;
    bool m_SearchAllDerivations;
    mitk::WeakPointer<mitk::DataStorage> m_DataStorage;
};
}