#pragma once

#include <QObject>
#include <QImage>

#include <mitkDataNode.h>
#include <mitkCameraController.h>

//////////////////////////////////////////////////////////////////////////
// Forward declarations

namespace mitk {
    class DataStorage;
} 
//////////////////////////////////////////////////////////////////////////

namespace crimson {

class ThumbnailGeneratorPrivate;

/*! \brief   Thumbnail generator creates the thumbnail images asynchronously for use in ContourModelingView. */
class ThumbnailGenerator : public QObject {
    Q_OBJECT
public:
    ThumbnailGenerator(mitk::DataStorage* dataStorage);
    ~ThumbnailGenerator();

    /*!
     * \brief   Add a node to the thumbnail generation queue.
     */
    void requestThumbnail(mitk::DataNode::ConstPointer planarFigureNode, mitk::TimePointType time = 0);

    /*!
     * \brief   Cancel the request to generate thumbnail for a node.
     */
    void cancelThumbnailRequest(mitk::DataNode::ConstPointer planarFigureNode);

    /*!
     * \brief   Get the renderer used by thumbnail generator.
     */
    mitk::BaseRenderer* getThumbnailRenderer();

signals:
    void thumbnailGenerated(mitk::DataNode::ConstPointer node, QImage thumbnail);

private:
    ThumbnailGenerator(const ThumbnailGenerator&) = delete;
    ThumbnailGenerator& operator=(const ThumbnailGenerator&) = delete;

    void _generateThumbnail(mitk::DataNode::ConstPointer planarFigureNode, mitk::TimePointType time);

    std::unique_ptr<ThumbnailGeneratorPrivate> d;

private slots:
    void _generateOneThumbnail();
};

} // namespace crimson