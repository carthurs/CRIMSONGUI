#include <NodePredicateDerivation.h>

namespace crimson
{

bool NodePredicateDerivation::CheckNode(const mitk::DataNode* node) const
{
    if (m_DataStorage && m_ParentNode) {
        const mitk::DataStorage::SetOfObjects::STLContainerType children =
            m_DataStorage->GetDerivations(m_ParentNode, nullptr, !m_SearchAllDerivations)->CastToSTLConstContainer();

        return std::find(children.begin(), children.end(), node) != children.end();
    }

    return false;
}

NodePredicateDerivation::NodePredicateDerivation(mitk::DataNode* n, bool allderiv, mitk::DataStorage* ds)
    : mitk::NodePredicateBase()
    , m_ParentNode(n)
    , m_SearchAllDerivations(allderiv)
    , m_DataStorage(ds)
{
}
}
