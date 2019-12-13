#pragma once

#include <berryIPerspectiveFactory.h>

namespace crimson {

class MeshingAndSolverSetupPerspective : public QObject, public berry::IPerspectiveFactory 
{
    Q_OBJECT
    Q_INTERFACES(berry::IPerspectiveFactory)

public:
    MeshingAndSolverSetupPerspective() {}
    ~MeshingAndSolverSetupPerspective() {}

    void CreateInitialLayout(berry::IPageLayout::Pointer /*layout*/) override;
};

} // namespace crimson
