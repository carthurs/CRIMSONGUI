#include "ExportVesselsAction.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include "VascularModelingUtils.h"

#include <VesselForestData.h>
#include <QtWidgets/QFileDialog>

#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkIdList.h>
#include <vtkPolyDataWriter.h>
#include <vtkSmartPointer.h>
#include <mitkPlanarFigure.h>
#include <mitkPlaneGeometry.h>

ExportVesselsAction::ExportVesselsAction()
{
}

ExportVesselsAction::~ExportVesselsAction()
{
}


void ExportVesselsAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
    QString outputFolder;

    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        auto hm = crimson::HierarchyManager::getInstance();

        if (!hm->getPredicate(crimson::VascularModelingNodeTypes::VesselTree())->CheckNode(node)) {
            continue;
        }

        if (outputFolder.isEmpty()) {
            outputFolder = QFileDialog::getExistingDirectory(nullptr, "Select output directory");

            if (outputFolder.isEmpty()) {
                return;
            }
        }

        auto* hierarchyManager = crimson::HierarchyManager::getInstance();

        // Find all the vessel paths in the vessel tree
        mitk::DataStorage::SetOfObjects::ConstPointer vesselPathNodes = hierarchyManager->getDescendants(node, crimson::VascularModelingNodeTypes::VesselPath(), true);

        for (const mitk::DataNode::Pointer& vesselPathNode : *vesselPathNodes) {
            auto vesselPath = static_cast<crimson::VesselPathAbstractData*>(vesselPathNode->GetData());

            if (!vesselPath->getPolyDataRepresentation()) {
                continue;
            }

            // Create VTK polyData
            vtkNew<vtkPolyData> outputPolyData;
            outputPolyData->DeepCopy(vesselPath->getPolyDataRepresentation());
            outputPolyData->GetPointData()->DeepCopy(vtkNew<vtkPointData>().Get());

            // Add contours to output
            std::vector<mitk::DataNode*> contourNodes = crimson::VascularModelingUtils::getVesselContourNodesSortedByParameter(vesselPathNode);

            for (const mitk::DataNode* planarFigureNode : contourNodes) {
                auto planarFigure = static_cast<mitk::PlanarFigure*>(planarFigureNode->GetData());

                mitk::PlanarFigure::PolyLineType polyLine = planarFigure->GetPolyLine(0);

                vtkNew<vtkIdList> contourIds;
                for (const auto& point2D : polyLine) {
                    mitk::Point3D point3D;
                    planarFigure->GetPlaneGeometry()->Map(point2D, point3D);

                    vtkIdType pointId = outputPolyData->GetPoints()->InsertNextPoint(point3D[0], point3D[1], point3D[2]);
                    contourIds->InsertNextId(pointId);
                }

                // Close the polyline
                contourIds->InsertNextId(contourIds->GetId(0));

                outputPolyData->InsertNextCell(VTK_POLY_LINE, contourIds.Get());
            }

            vtkNew<vtkPolyDataWriter> writer;
            writer->SetInputData(outputPolyData.Get());
            writer->SetFileName(QString("%1/%2.vtk").arg(outputFolder).arg(vesselPathNode->GetName().c_str()).toLatin1());
            writer->Update();
        }
    }
}
