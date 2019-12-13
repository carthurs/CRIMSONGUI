#include "CRIMSONWorkflowWidget.h"

#include "ExpandablePerspectiveButton.h"
#include "PopupWidget.h"

#include <berryPlatformUI.h>
#include <berryIWorkbenchPage.h>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QButtonGroup>

namespace crimson {

class CRIMSONWorkflowWidgetPerspectiveListener : public berry::IPerspectiveListener
{
public:
    CRIMSONWorkflowWidgetPerspectiveListener(QVector<ExpandablePerspectiveButtonFrame*> perspectiveButtons, QButtonGroup* buttonGroup) :
        perspectiveButtons(perspectiveButtons),
        buttonGroup(buttonGroup)
    {
    }

    Events::Types GetPerspectiveEventTypes() const override
    {
        return Events::ACTIVATED | Events::DEACTIVATED;
    }

    void PerspectiveActivated(const berry::SmartPointer<berry::IWorkbenchPage>& page,
        const berry::IPerspectiveDescriptor::Pointer& perspective) override
    {
        bool anyPerspectiveActivated = false;
        for (ExpandablePerspectiveButtonFrame* expandablePerspectiveButtonFrame : perspectiveButtons) {
            if (expandablePerspectiveButtonFrame->getPerspectiveName() == perspective->GetId().toStdString()) {
                buttonGroup->setExclusive(true);
                expandablePerspectiveButtonFrame->getPerspectiveButton()->setChecked(true);
                anyPerspectiveActivated = true;
                break;
            }
        }

        if (!anyPerspectiveActivated) {
            for (ExpandablePerspectiveButtonFrame* expandablePerspectiveButtonFrame : perspectiveButtons) {
                buttonGroup->setExclusive(false);
                expandablePerspectiveButtonFrame->getPerspectiveButton()->setChecked(false);
            }
        }
    }

    void PerspectiveDeactivated(const berry::SmartPointer<berry::IWorkbenchPage>& page,
        const berry::IPerspectiveDescriptor::Pointer& perspective) override
    {
        for (ExpandablePerspectiveButtonFrame* expandablePerspectiveButtonFrame : perspectiveButtons) {
            if (expandablePerspectiveButtonFrame->getPerspectiveName() == perspective->GetId().toStdString()) {
                expandablePerspectiveButtonFrame->getPerspectiveButton()->setChecked(false);
            }
        }
    }

private:
    QVector<ExpandablePerspectiveButtonFrame*> perspectiveButtons;
    QButtonGroup* buttonGroup;
};

struct CRIMSONWorkflowWidgetWindowListener : public berry::IWindowListener
{
    CRIMSONWorkflowWidgetWindowListener(const QVector<ExpandablePerspectiveButtonFrame*>& perspectiveButtons, QButtonGroup* buttonGroup)
        : _done(false)
        , _perspectiveListener(new CRIMSONWorkflowWidgetPerspectiveListener(perspectiveButtons, buttonGroup))
    {
    }

    virtual void WindowOpened(const berry::IWorkbenchWindow::Pointer& window) override
    {
        if (_done)
            return;
        _done = true;
        window->AddPerspectiveListener(_perspectiveListener.data());
    }

    virtual void WindowActivated(const berry::IWorkbenchWindow::Pointer& window) override
    {
        if (_done)
            return;
        _done = true;
        window->AddPerspectiveListener(_perspectiveListener.data());
    }

private:
    bool _done;
    QScopedPointer<CRIMSONWorkflowWidgetPerspectiveListener> _perspectiveListener;
};



CRIMSONWorkflowWidget::CRIMSONWorkflowWidget(const std::vector<PerspectiveAndViewsType>& perspectivesAndViews, QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);

    QButtonGroup* buttonGroup = new QButtonGroup(this);
    QVector<ExpandablePerspectiveButtonFrame*> perspectiveButtons;
    for (const std::pair<std::string, std::vector<std::string>>& perspectiveAndViews : perspectivesAndViews) {
        auto expandablePerspectiveButtonFrame = new ExpandablePerspectiveButtonFrame(perspectiveAndViews.first, perspectiveAndViews.second, this);
        layout->addWidget(expandablePerspectiveButtonFrame);
        buttonGroup->addButton(expandablePerspectiveButtonFrame->getPerspectiveButton());
        perspectiveButtons.push_back(expandablePerspectiveButtonFrame);
    }

    layout->addItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    layout->setStretch(perspectiveButtons.size(), 1);
    this->setLayout(layout);

    _windowListener.reset(new CRIMSONWorkflowWidgetWindowListener(perspectiveButtons, buttonGroup));
    berry::PlatformUI::GetWorkbench()->AddWindowListener(_windowListener.data());
}

void CRIMSONWorkflowWidget::collapseAll()
{
    for (ExpandablePerspectiveButtonFrame* expandablePerspectiveButtonFrame : this->findChildren<ExpandablePerspectiveButtonFrame*>()) {
        PopupWidget* popupWidget = expandablePerspectiveButtonFrame->getPopupWidget();
        popupWidget->setEffectDuration(0);
        popupWidget->hidePopup();
        popupWidget->setEffectDuration(300);
    }
}

} // namespace crimson

