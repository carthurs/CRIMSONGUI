#pragma once

#include <functional>

#include <berryIPartListener.h>
#include <berryIWorkbenchPartSite.h>
#include <berryIWorkbenchPage.h>

#include "VesselDrivenResliceView.h"

namespace crimson {

//////////////////////////////////////////////////////////////////////////
// The listener for the reslice view. 
class VesselDrivenResliceViewListener : public berry::IPartListener {
public:
    berryObjectMacro(VesselDrivenResliceViewListener);

    VesselDrivenResliceViewListener(std::function<void(VesselDrivenResliceView*)> setVesselDrivenResliceView, berry::IWorkbenchPartSite* viewSite)
        : _setVesselDrivenResliceView(setVesselDrivenResliceView)
        , _viewSite(viewSite)
    {
        // Check if vessel driven view exists
        auto activePage = viewSite->GetWorkbenchWindow()->GetActivePage();
        if (activePage) {
            auto resliceView = static_cast<VesselDrivenResliceView*>(activePage->FindView(QString::fromStdString(VesselDrivenResliceView::VIEW_ID)).GetPointer());
            _setVesselDrivenResliceView(resliceView);
        }
    }

    void registerListener()
    {
        _viewSite->GetPage()->AddPartListener(this);
    }

    void unregisterListener()
    {
        _viewSite->GetPage()->RemovePartListener(this);
    }

    Events::Types GetPartEventTypes() const override
    {
        return Events::HIDDEN | Events::VISIBLE;
    }

    void PartHidden(const berry::IWorkbenchPartReference::Pointer& partRef) override
    {
        if (partRef->GetId() == QString::fromStdString(VesselDrivenResliceView::VIEW_ID)) {
            _setVesselDrivenResliceView(nullptr);
        }
    }

    void PartVisible(const berry::IWorkbenchPartReference::Pointer& partRef) override
    {
        if (partRef->GetId() == QString::fromStdString(VesselDrivenResliceView::VIEW_ID)) {
            auto vesselDrivenView = partRef->GetPart(false).Cast<VesselDrivenResliceView>();
            _setVesselDrivenResliceView(vesselDrivenView.GetPointer());
        }
    }

private:
    std::function<void(VesselDrivenResliceView*)> _setVesselDrivenResliceView;
    berry::IWorkbenchPartSite* _viewSite;
};

} // namespace crimson