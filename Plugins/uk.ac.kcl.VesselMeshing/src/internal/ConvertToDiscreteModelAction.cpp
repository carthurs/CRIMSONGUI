#include "ConvertToDiscreteModelAction.h"

#include <mitkDataStorage.h>
#include <DiscreteSolidData.h>
#include <HierarchyManager.h>

ConvertToDiscreteModelAction::ConvertToDiscreteModelAction()
{
}

ConvertToDiscreteModelAction::~ConvertToDiscreteModelAction()
{
}


void ConvertToDiscreteModelAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        mitk::Surface::Pointer surface = dynamic_cast<mitk::Surface*>(node->GetData());
        if (!surface) {
            continue;
        }

        std::string name = node->GetName();

        crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(node);

        crimson::DiscreteSolidData::Pointer solidData = crimson::DiscreteSolidData::New();
        solidData->fromSurface(surface);
        node->SetData(solidData.GetPointer());
        node->SetName(name);

        crimson::HierarchyManager::getInstance()->getDataStorage()->Add(node);
    }
}