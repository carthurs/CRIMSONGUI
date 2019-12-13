#pragma once

#include <QFrame>
#include <QToolButton>

#include "PopupWidget.h"

class ExpandablePerspectiveButtonFrame : public QFrame
{
    Q_OBJECT
public:
    ExpandablePerspectiveButtonFrame(const std::string& perspectiveName, const std::vector<std::string>& viewNames,
                                     QWidget* parent = nullptr, Qt::WindowFlags flags = 0);

    QAbstractButton* getPerspectiveButton() { return _perspectiveButton; }
    const std::string& getPerspectiveName() const { return _perspectiveName; }

    PopupWidget* getPopupWidget() { return _popupWidget; }

private:
    QToolButton* _perspectiveButton;
    std::string _perspectiveName;

    PopupWidget* _popupWidget;
};