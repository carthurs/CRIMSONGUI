#pragma once

#include <berryIPerspectiveFactory.h>

namespace crimson {

class GeometryModelingPerspective : public QObject, public berry::IPerspectiveFactory 
{
    Q_OBJECT
    Q_INTERFACES(berry::IPerspectiveFactory)

public:
    GeometryModelingPerspective() {}
    ~GeometryModelingPerspective() {}

    void CreateInitialLayout(berry::IPageLayout::Pointer /*layout*/) override;
};

} // namespace crimson
