#pragma once

#include <QWidget>
#include <berryIWindowListener.h>

namespace crimson {

class CRIMSONWorkflowWidget : public QWidget {
    Q_OBJECT
public:
    using PerspectiveAndViewsType = std::pair<std::string, std::vector<std::string>>;

    CRIMSONWorkflowWidget(const std::vector<PerspectiveAndViewsType>& perspectivesAndViews, QWidget* parent = nullptr);

    // This should be called after the widget is shown to correctly collapse all buttons
    void collapseAll();
private:
    QScopedPointer<berry::IWindowListener> _windowListener;
};

} // namespace crimson