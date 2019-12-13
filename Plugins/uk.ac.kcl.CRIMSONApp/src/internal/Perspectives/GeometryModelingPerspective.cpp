#include "GeometryModelingPerspective.h"

#include <berryIViewLayout.h>

namespace crimson {

void GeometryModelingPerspective::CreateInitialLayout(berry::IPageLayout::Pointer layout)
{
    QString editorArea = layout->GetEditorArea();

    layout->AddView("org.mitk.views.datamanager",
        berry::IPageLayout::LEFT, 0.2f, editorArea);

    berry::IViewLayout::Pointer lo = layout->GetViewLayout("org.mitk.views.datamanager");
    lo->SetCloseable(false);

    layout->AddView("org.mitk.views.imagenavigator",
        berry::IPageLayout::BOTTOM, 0.5f, "org.mitk.views.datamanager");

    layout->AddView("org.mitk.views.vesselpathplanningview",
        berry::IPageLayout::RIGHT, 0.7f, editorArea);

    auto folder = static_cast<berry::IFolderLayout*>(layout->GetFolderForView("org.mitk.views.vesselpathplanningview").GetPointer());
    folder->AddView("org.mitk.views.ContourModelingView");

    layout->AddView("org.mitk.views.VesselDrivenResliceView",
        berry::IPageLayout::TOP, 0.3f, editorArea);

}

} // namespace crimson