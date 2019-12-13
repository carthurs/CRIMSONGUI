#pragma once

#include <mitkNodePredicateBase.h>

namespace crimson {

/*! \brief   An empty node predicate. */
class NodePredicateNone : public mitk::NodePredicateBase
{
public:
    mitkClassMacro(NodePredicateNone, NodePredicateBase);

    itkFactorylessNewMacro(NodePredicateNone);
    bool CheckNode(const mitk::DataNode*) const override { return false; }
private:
    NodePredicateNone() {}
};

}