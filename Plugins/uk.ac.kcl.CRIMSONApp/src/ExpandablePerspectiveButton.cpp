#include <mitkLogMacros.h>

#include "ExpandablePerspectiveButton.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>

#include "PopupWidget.h"

#include <berryIViewRegistry.h>
#include <berryPlatformUI.h>
#include <berryIWorkbenchPage.h>

ExpandablePerspectiveButtonFrame::ExpandablePerspectiveButtonFrame(const std::string& perspectiveName,
                                                                   const std::vector<std::string>& viewNames, QWidget* parent,
                                                                   Qt::WindowFlags flags)
    : QFrame(parent, flags)
    , _perspectiveName(perspectiveName)
{
    auto horizontalLayoutButtonWithPopup = new QHBoxLayout();

    _perspectiveButton = new QToolButton(this);
    _perspectiveButton->setText(QString::fromStdString(perspectiveName));
    _perspectiveButton->setToolTip(QString::fromStdString(perspectiveName));
    _perspectiveButton->setCheckable(true);
    _perspectiveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    berry::IPerspectiveRegistry* perspectiveRegistry = berry::PlatformUI::GetWorkbench()->GetPerspectiveRegistry();
    berry::IPerspectiveDescriptor::Pointer perspectiveDescriptor =
        perspectiveRegistry->FindPerspectiveWithId(QString::fromStdString(perspectiveName));

    if (perspectiveDescriptor.IsNotNull()) {
        _perspectiveButton->setText(perspectiveDescriptor->GetLabel());
        _perspectiveButton->setIcon(perspectiveDescriptor->GetImageDescriptor());
        _perspectiveButton->setIconSize(QSize(32, 32));
    }

    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    _perspectiveButton->setSizePolicy(sizePolicy);

    _perspectiveButton->setStyleSheet("QToolButton {"
                                      "color: #ffffff;" // Text color
                                      "border: 0px;"
                                      "background-color: rgba(0,0,0,0);" // Transparent button
                                      "padding-left: 10px;"
                                      "padding-right: 0px;"
                                      "}");
    _perspectiveButton->setFixedHeight(60);

    this->setObjectName("ExpandablePerspectiveButtonFrame");
    horizontalLayoutButtonWithPopup->setSpacing(0);
    horizontalLayoutButtonWithPopup->setContentsMargins(0, 0, 10, 0);

    auto setStyleSheetLambda = [this](bool on) {
        this->setStyleSheet(QString("QFrame#ExpandablePerspectiveButtonFrame {"
                                    "border: 1px solid rgba(50, 200, 50, 200);"
                                    "border-radius: 6px;"
                                    "background-color: qlineargradient(spread : pad, x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : %1 "
                                    "rgba(0, 100, 0, 255), stop : %2 rgba(50, 200, 50, 255));"
                                    "}")
                                .arg(on ? 0 : 1)
                                .arg(on ? 1 : 0) // Invert gradient when button is pushed
                            );
    };
    setStyleSheetLambda(false);

    connect(_perspectiveButton, &QAbstractButton::toggled, [this, perspectiveName, setStyleSheetLambda](bool on) {
        if (on) {
            berry::PlatformUI::GetWorkbench()->ShowPerspective(QString::fromStdString(perspectiveName),
                                                               berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow());
        }
        setStyleSheetLambda(on);
    });

    horizontalLayoutButtonWithPopup->addWidget(_perspectiveButton);

    _popupWidget = new PopupWidget(this);
    _popupWidget->setSizePolicy(sizePolicy);
    _popupWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    _popupWidget->setOrientation(Qt::Horizontal);

    QObject::connect(_perspectiveButton, SIGNAL(toggled(bool)), _popupWidget, SLOT(showPopup(bool)));

    auto horizontalLayoutPopup = new QHBoxLayout(_popupWidget->frame());
    horizontalLayoutPopup->setContentsMargins(7, 0, 0, 0);

    berry::IViewRegistry* viewRegistry = berry::PlatformUI::GetWorkbench()->GetViewRegistry();

    for (const std::string& viewName : viewNames) {
        berry::IViewDescriptor::Pointer viewDescriptor = viewRegistry->Find(QString::fromStdString(viewName));

        if (viewDescriptor.IsNull()) {
            MITK_ERROR << "Failed to find view descriptor for view " << viewName;
            continue;
        }

        auto tb = new QToolButton(_popupWidget);
        tb->setText(viewDescriptor->GetLabel());
        tb->setToolTip(viewDescriptor->GetLabel());
        tb->setIcon(viewDescriptor->GetImageDescriptor());
        tb->setIconSize(QSize(32, 32));
        tb->setSizePolicy(sizePolicy);
        tb->setAutoRaise(true);
        //        tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        horizontalLayoutPopup->addWidget(tb);

        connect(tb, &QAbstractButton::clicked, [viewName]() {
            berry::IWorkbenchPage::Pointer page =
                berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow()->GetActivePage();
            if (page.IsNotNull()) {
                page->ShowView(QString::fromStdString(viewName));
            }
        });
    }

    _popupWidget->setObjectName("PopupWidget");
    _popupWidget->setStyleSheet("QWidget#PopupWidget {background-color: rgba(0,0,0,0);}"
                                "QWidget#PopupWidgetFramePage {background-color: rgba(0,0,0,0);}"
                                "QWidget#PopupPixmapWidget {background-color: rgba(0,0,0,0);}"
                                "QWidget#PopupPixmapWidgetPage {background-color: rgba(0,0,0,0);}");
    _popupWidget->frame()->setStyleSheet(
        "QFrame#PopupWidgetFrame { border: 0px; background-color:#323232; border-radius: 6px; }");
    horizontalLayoutPopup->setContentsMargins(7, 5, 7, 5);
    horizontalLayoutPopup->setSpacing(7);

    horizontalLayoutButtonWithPopup->addWidget(_popupWidget);

    this->setLayout(horizontalLayoutButtonWithPopup);
}
