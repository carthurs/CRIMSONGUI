#pragma once

#include <QtVariantPropertyManager>

namespace crimson {

/*! \brief   A class, managing the user-editable properties of an object in QtPropertyEditor format. */
class QtPropertyStorage {
public:
    QtPropertyStorage() { _topProperty = _propertyManager.addProperty(QtVariantPropertyManager::groupTypeId()); }
    QtPropertyStorage(const QtPropertyStorage& other)
    {
        *this = other;
    }

    QtPropertyStorage& operator=(const QtPropertyStorage& other)
    {
        if (this == &other) {
            return *this;
        }

        _propertyManager.setProperties(other._propertyManager.properties());

        foreach(QtProperty* prop, _propertyManager.properties()) {
            if (prop->propertyName() == other._topProperty->propertyName()) {
                _topProperty = _propertyManager.variantProperty(prop);
                break;
            }
        }

        return *this;
    }

    /*!
     * \brief   Gets property QtVariantPropertyManager with all the properties.
     */
    QtVariantPropertyManager* getPropertyManager() { return &_propertyManager; }

    /*!
     * \brief   Gets top-level group property.
     */
    QtProperty* getTopProperty() { return _topProperty; }

    /*!
     * \brief   Gets property value by the property name.
     */
    QVariant getPropertyValueByName(const QString& name) const
    {
        foreach(QtProperty* prop, _propertyManager.properties()) {
            if (prop->propertyName() == name) {
                return _propertyManager.value(prop);
            }
        }
        return QVariant();
    }

private:
    QtVariantPropertyManager _propertyManager;
    QtVariantProperty* _topProperty;
};

}
