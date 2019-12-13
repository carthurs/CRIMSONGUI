#pragma once

#include <QObject>
#include <QVariant>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>

#include <IMeshingKernel.h>

#include "CGALVMTKMeshingKernelExports.h"


/*! \brief   Values that represent status of local meshing parameters as compared to global ones. */
enum OverrideStatus {
    StatusNotOverriden = 0, ///< The global parameters are not overriden
    StatusOverriden,    ///< The global parameters are overriden, but not conflicting
    StatusConflicting,  ///< For multiple selection, local overrides for different faces have different values

    StatusLast
};

/*! \brief   A class that sets the status-dependent icon for a button, and resets the status to 'not overriden' 
 * if the button is pressed. 
 */
class CGALVMTKMeshingKernel_EXPORT OverrideStatusButtonHandler : public QObject {
public:
    Q_OBJECT
public:

    /*!
     * \brief   Constructor.
     *
     * \param   initialStatus           The initial override status.
     * \param   overrideStatusButton    If non-null, the set use default button.
     * \param   editingGlobal           If true (editing global parameters), the button is hidden.
     * \param   parent                  (Optional) If non-null, the parent.
     */
    OverrideStatusButtonHandler(OverrideStatus initialStatus, QAbstractButton* overrideStatusButton, bool editingGlobal,
                                QObject* parent = nullptr)
        : QObject(parent)
        , _status(initialStatus)
        , _overrideStatusButton(overrideStatusButton)
    {

        if (editingGlobal) {
            overrideStatusButton->setVisible(false);
        }

        updateUseDefaultsButton();
        emit statusChanged(_status);

        connect(overrideStatusButton, &QAbstractButton::clicked, this, &OverrideStatusButtonHandler::setToNotOverriden);
    }

    void updateUseDefaultsButton()
    {
        static const char* iconNames[] = { ":/icons/default", ":/icons/use_default", ":/icons/warning" };
        static QString toolTips[] = { 
            tr("There are no local parameters set for these faces - globals will be used."),
            tr("Press this button to use the global parameter value."),
            tr("Values set for different faces are conflicting.\n"
                "Press this button to use the global parameter value or edit the value to set it for all faces."),
        };

        _overrideStatusButton->setIcon(QIcon(iconNames[_status]));
        _overrideStatusButton->setToolTip(toolTips[_status]);
    }

    OverrideStatus status() const { return _status; }

public slots:
    void setToNotOverriden() { setStatus(StatusNotOverriden); }
    void setStatus(OverrideStatus status)
    {
        if (_status == status) {
            return;
        }
        _status = status;

        updateUseDefaultsButton();
        emit statusChanged(_status);
    }

signals:
    void statusChanged(OverrideStatus status);

private:
    OverrideStatus _status;
    QAbstractButton* _overrideStatusButton;
};

class CGALVMTKMeshingKernel_EXPORT DefaultValuedEditor : public QObject {
    Q_OBJECT
public:
    DefaultValuedEditor(QObject* parent = nullptr)
        : QObject(parent) {}

public slots:
    virtual void setToStatus(OverrideStatus s) = 0;
signals:
    void valueSet();
};

/*! \brief   Support for setting default value to a set of autoexclusive radio buttons. */
class CGALVMTKMeshingKernel_EXPORT AutoExclusiveButtonDefaultValuedEditor : public DefaultValuedEditor {
    Q_OBJECT
public:
    AutoExclusiveButtonDefaultValuedEditor(const std::vector<QAbstractButton*>& autoExclusiveButtons, QAbstractButton* defaultValueButton, QObject* parent = nullptr)
        : DefaultValuedEditor(parent)
        , _autoExclusiveButtons(autoExclusiveButtons)
        , _defaultValueButton(defaultValueButton)
    {
        for (QAbstractButton* button : _autoExclusiveButtons) {
            connect(button, &QAbstractButton::toggled, this, &AutoExclusiveButtonDefaultValuedEditor::resetAutoexclusive);
        }
    }

public slots:
    void setToStatus(OverrideStatus s) override
    {
        for (QAbstractButton* button : _autoExclusiveButtons) {
            disconnect(button, &QAbstractButton::toggled, this, &AutoExclusiveButtonDefaultValuedEditor::resetAutoexclusive);
        }

        if (s == StatusConflicting) {
            // Uncheck all if status is conflicting
            for (QAbstractButton* button : _autoExclusiveButtons) {
                button->setAutoExclusive(false);
                button->setChecked(false);
            }
        }
        else if (s == StatusNotOverriden) {
            for (QAbstractButton* button : _autoExclusiveButtons) {
                button->setAutoExclusive(true);
            }
            _defaultValueButton->setChecked(true);
        }

        for (QAbstractButton* button : _autoExclusiveButtons) {
            connect(button, &QAbstractButton::toggled, this, &AutoExclusiveButtonDefaultValuedEditor::resetAutoexclusive);
        }
    }

    void resetAutoexclusive() 
    {
        for (QAbstractButton* button : _autoExclusiveButtons) {
            button->setAutoExclusive(true);
        }
        emit valueSet();
    }

private:
    std::vector<QAbstractButton*> _autoExclusiveButtons;
    QAbstractButton* _defaultValueButton;
};

/*! \brief   Support for setting default value to check box. */
class CGALVMTKMeshingKernel_EXPORT CheckboxDefaultValuedEditor : public DefaultValuedEditor {
    Q_OBJECT
public:
    CheckboxDefaultValuedEditor(QCheckBox* cb, bool defaultValue, QObject* parent = nullptr)
        : DefaultValuedEditor(parent)
        , _cb(cb)
        , _defaultValue(defaultValue)
    {
        connect(_cb, &QCheckBox::stateChanged, this, &CheckboxDefaultValuedEditor::resetTwoState);
    }

public slots:
    void setToStatus(OverrideStatus s) override
    {
        disconnect(_cb, &QCheckBox::stateChanged, this, &CheckboxDefaultValuedEditor::resetTwoState);
        if (s == StatusConflicting) {
            // Set to partially checked if conflicting
            _cb->setTristate(true);
            _cb->setCheckState(Qt::PartiallyChecked);
        }
        else if (s == StatusNotOverriden) {
            _cb->setTristate(false);
            _cb->setChecked(_defaultValue);
        }
        connect(_cb, &QCheckBox::stateChanged, this, &CheckboxDefaultValuedEditor::resetTwoState);
    }

    void resetTwoState()
    {
        _cb->setTristate(false);
        emit valueSet();
    }

private:
    QCheckBox* _cb;
    bool _defaultValue;
};

/*! \brief   Support for setting default value to spin box. */
class CGALVMTKMeshingKernel_EXPORT SpinBoxDefaultValuedEditor : public DefaultValuedEditor {
    Q_OBJECT
public:
    // QWidget* - to support both QDoubleSpinBox and ctkDoubleSpinBox
    SpinBoxDefaultValuedEditor(QWidget* sb, double defaultValue, QObject* parent = nullptr)
        : DefaultValuedEditor(parent)
        , _sb(sb)
        , _defaultValue(defaultValue)
    {
        connect(_sb, SIGNAL(valueChanged(const QString&)), this, SIGNAL(valueSet()));
    }

public slots:
    void setToStatus(OverrideStatus s) override
    {

        disconnect(_sb, SIGNAL(valueChanged(const QString&)), this, SIGNAL(valueSet()));

        if (s == StatusConflicting) {
            // Set to minimum value if conflicting
            // Using invokeMethod to call a slot regardless of whether _sb is a QDoubleSpinBox or a ctkDoubleSpinBox
            QMetaObject::invokeMethod(_sb, "setValue", Qt::DirectConnection, Q_ARG(double, _sb->property("minimumValue").toDouble()));
        }
        else if (s == StatusNotOverriden) {
            QMetaObject::invokeMethod(_sb, "setValue", Qt::DirectConnection, Q_ARG(double, _defaultValue));
        }

        connect(_sb, SIGNAL(valueChanged(const QString&)), this, SIGNAL(valueSet()));
    }

private:
    QWidget* _sb;
    double _defaultValue;
};

/*! \brief   Support for setting default value to combo box. */
class CGALVMTKMeshingKernel_EXPORT ComboBoxDefaultValuedEditor : public DefaultValuedEditor {
    Q_OBJECT
public:
    ComboBoxDefaultValuedEditor(QComboBox* cb, int defaultIndex, QObject* parent = nullptr)
        : DefaultValuedEditor(parent)
        , _cb(cb)
        , _defaultIndex(defaultIndex)
    {
        connect(_cb, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueSet()));
    }

public slots:
    void setToStatus(OverrideStatus s) override
    {
        disconnect(_cb, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueSet()));

        if (s == StatusConflicting) {
            // Set to the first value if conflicting
            _cb->setCurrentIndex(0);
        }
        else if (s == StatusNotOverriden) {
            _cb->setCurrentIndex(_defaultIndex);
        }

        connect(_cb, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueSet()));
    }

private:
    QComboBox* _cb;
    int _defaultIndex;
};

/*! \brief   Editor for a local parameter value which combines a DefaultValuedEditor with OverrideStatusButtonHandler. */
class CGALVMTKMeshingKernel_EXPORT LocalParameterValueEditor : public QObject {
    Q_OBJECT
public:
    LocalParameterValueEditor(OverrideStatusButtonHandler* statusHandler, DefaultValuedEditor* defaultValueEditor, QObject* parent = nullptr)
        : QObject(parent)
        , _statusHandler(statusHandler)
        , _defaultValueEditor(defaultValueEditor)
    {
        connect(statusHandler, &OverrideStatusButtonHandler::statusChanged, _defaultValueEditor, &DefaultValuedEditor::setToStatus);
        connect(_defaultValueEditor, &DefaultValuedEditor::valueSet, this, &LocalParameterValueEditor::setToNormalState);

        defaultValueEditor->setToStatus(statusHandler->status());
    }

public slots:
    void setToNormalState()
    {
        _statusHandler->setStatus(StatusOverriden);
    }

private:
    OverrideStatusButtonHandler* _statusHandler;
    DefaultValuedEditor* _defaultValueEditor;
};

