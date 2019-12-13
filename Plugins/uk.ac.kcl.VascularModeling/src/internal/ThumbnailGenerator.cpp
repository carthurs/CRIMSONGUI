#include "ThumbnailGenerator.h"

#include <unordered_map>

#include <QTimer>
#include <QTime>
#include <QApplication>
#include <QDesktopWidget>

#include <vtkNew.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageData.h>

#include <QmitkRenderWindow.h>
#include <mitkRenderWindow.h>
#include <mitkPlanarFigure.h>
#include <mitkProportionalTimeGeometry.h>

namespace crimson {

struct DataNodeConstPointerHasher {
    std::size_t operator()(mitk::DataNode::ConstPointer ptr) const { return std::hash<const mitk::DataNode*>()(ptr.GetPointer()); }
};

class ThumbnailGeneratorPrivate {
public:
#ifdef Q_OS_LINUX
    QmitkRenderWindow* thumbnailRenderwindow;
    bool firstRender = true;
#else 
    mitk::RenderWindow::Pointer thumbnailRenderwindow;
#endif
    QTimer thumbnailUpdateTimer;

    struct ThumbnailRequestInfo {
        QTime requestTime;
        mitk::TimePointType time;
    };

    std::unordered_map<mitk::DataNode::ConstPointer, ThumbnailRequestInfo, DataNodeConstPointerHasher> thumbnailUpdateMap;
};

ThumbnailGenerator::ThumbnailGenerator(mitk::DataStorage* dataStorage)
    : d(new ThumbnailGeneratorPrivate)
{
#ifdef Q_OS_LINUX
    d->thumbnailRenderwindow = new QmitkRenderWindow();

    d->thumbnailRenderwindow->setFixedSize(128, 128); 
    d->thumbnailRenderwindow->setWindowFlags(Qt::ToolTip | Qt::Dialog);
#else
    vtkRenderWindow* renWin = vtkRenderWindow::New(); // Non-smart pointer is intentional
    renWin->OffScreenRenderingOn();
    renWin->BordersOff();

    d->thumbnailRenderwindow = mitk::RenderWindow::New(renWin);
    d->thumbnailRenderwindow->SetSize(128, 128);
#endif

    d->thumbnailRenderwindow->GetRenderer()->SetDataStorage(dataStorage);

    d->thumbnailUpdateTimer.start(100); 
    connect(&d->thumbnailUpdateTimer, SIGNAL(timeout()), this, SLOT(_generateOneThumbnail()));

    mitk::RenderingManager::GetInstance()->RemoveRenderWindow(d->thumbnailRenderwindow->GetVtkRenderWindow());
}

ThumbnailGenerator::~ThumbnailGenerator()
{
}

void ThumbnailGenerator::requestThumbnail(mitk::DataNode::ConstPointer planarFigureNode, mitk::TimePointType time)
{
    d->thumbnailUpdateMap[planarFigureNode].requestTime.restart();
    d->thumbnailUpdateMap[planarFigureNode].time = time;
}

void ThumbnailGenerator::cancelThumbnailRequest(mitk::DataNode::ConstPointer planarFigureNode)
{
    d->thumbnailUpdateMap.erase(planarFigureNode);
}

mitk::BaseRenderer* ThumbnailGenerator::getThumbnailRenderer()
{
    return d->thumbnailRenderwindow->GetRenderer();
}

void ThumbnailGenerator::_generateThumbnail(mitk::DataNode::ConstPointer planarFigureNode, mitk::TimePointType time)
{
    mitk::VtkPropRenderer* renderer = d->thumbnailRenderwindow->GetRenderer();

    // Set renderer geometry to the planar figure's geometry
    mitk::PlaneGeometry::Pointer geo = static_cast<mitk::PlanarFigure*>(planarFigureNode->GetData())->GetPlaneGeometry()->Clone();
    geo->SetReferenceGeometry(geo);
    renderer->SetWorldGeometry3D(geo);
    renderer->GetCameraController()->Fit();
    auto timeGeometry = dynamic_cast<mitk::ProportionalTimeGeometry*>(renderer->GetWorldTimeGeometry());
    if (timeGeometry) {
        timeGeometry->SetFirstTimePoint(time);
    }

    mitk::RenderingManager::GetInstance()->AddRenderWindow(d->thumbnailRenderwindow->GetVtkRenderWindow());
    const_cast<mitk::DataNode*>(planarFigureNode.GetPointer())->SetVisibility(true, renderer);


#ifdef Q_OS_LINUX
    // Linux rendering issues workarounds
    QRect rec = QApplication::desktop()->screenGeometry();
    d->thumbnailRenderwindow->move(rec.width() - d->thumbnailRenderwindow->width(), rec.height() - d->thumbnailRenderwindow->height());
    d->thumbnailRenderwindow->show();

    // Render the data
    if (d->firstRender) {
        d->firstRender = false;
        renderer->ForceImmediateUpdate();
    }
#endif

    // Render the data
    renderer->ForceImmediateUpdate();

    // Save the renderer output as a QImage
    vtkNew<vtkWindowToImageFilter> imageFilter;
    imageFilter->SetInput(d->thumbnailRenderwindow->GetVtkRenderWindow());
    imageFilter->SetInputBufferTypeToRGB();
    imageFilter->ShouldRerenderOff();
    imageFilter->ReadFrontBufferOff();
    imageFilter->Update();

#ifdef Q_OS_LINUX
    // No support for off-screen rendering in Linux. Remove the render window from screen.
    d->thumbnailRenderwindow->hide();
#endif

    mitk::RenderingManager::GetInstance()->RemoveRenderWindow(d->thumbnailRenderwindow->GetVtkRenderWindow());

    // Extract the rendering result
    vtkImageData* imageData = imageFilter->GetOutput();
    QImage img(imageData->GetDimensions()[0], imageData->GetDimensions()[1], QImage::Format_RGB888);
    img.fill(Qt::red);
    for (int j = 0; j < img.height(); j++) {
        for (int i = 0; i < img.width(); i++) {
            auto pixel = static_cast<unsigned char*>(imageData->GetScalarPointer(i, j, 0));
            img.setPixel(i, img.height() - j - 1, qRgb(pixel[0], pixel[1], pixel[2]));
        }
    }

    // Notify the listeners
    emit thumbnailGenerated(planarFigureNode, img);
}

void ThumbnailGenerator::_generateOneThumbnail()
{
    if (d->thumbnailUpdateMap.empty()) {
        return;
    }

    for (auto iter = d->thumbnailUpdateMap.begin(); iter != d->thumbnailUpdateMap.end(); ++iter) {
        if (iter->second.requestTime.elapsed() > 300) { // Prevent continuous updates when the contour is being modified
            _generateThumbnail(iter->first, iter->second.time);
            d->thumbnailUpdateMap.erase(iter);
            return;
        }
    }
}


} // namespace crimson
