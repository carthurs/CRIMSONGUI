#pragma once

#include <functional>

#include <berryIPartListener.h>
#include <berryIWorkbenchPartSite.h>
#include <berryIWorkbenchPage.h>

#include "ResliceView.h"

namespace crimson {

//////////////////////////////////////////////////////////////////////////
// The listener for the reslice view. 
class ResliceViewListener : public berry::IPartListener {
public:
    berryObjectMacro(ResliceViewListener);

    ResliceViewListener(std::function<void(ResliceView*)> setResliceView, berry::IWorkbenchPartSite* viewSite)
        : _setResliceView(setResliceView)
        , _viewSite(viewSite)
    {
        // Check if reslice view exists
        auto activePage = viewSite->GetWorkbenchWindow()->GetActivePage();
        if (activePage) {
            auto resliceView = static_cast<ResliceView*>(activePage->FindView(QString::fromStdString(ResliceView::VIEW_ID)).GetPointer());
            _setResliceView(resliceView);
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
        if (partRef->GetId() == QString::fromStdString(ResliceView::VIEW_ID)) {
            _setResliceView(nullptr);
        }
    }

    void PartVisible(const berry::IWorkbenchPartReference::Pointer& partRef) override
    {
        if (partRef->GetId() == QString::fromStdString(ResliceView::VIEW_ID)) {
            auto vesselDrivenView = partRef->GetPart(false).Cast<ResliceView>();
            _setResliceView(vesselDrivenView.GetPointer());
        }
    }

private:
    std::function<void(ResliceView*)> _setResliceView;
    berry::IWorkbenchPartSite* _viewSite;
};

} // namespace crimson