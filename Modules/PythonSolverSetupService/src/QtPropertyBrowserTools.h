#pragma once

#include <QtVariantPropertyManager>
#include <PythonQtObjectPtr.h>
#include <PythonQt.h>

namespace crimson
{

class QtPropertyBrowserTools
{
public:
    static void fillPropertiesFromPyObject(QtVariantPropertyManager* propertyManager, QtProperty* topProperty,
                                           PythonQtObjectPtr pyObject, const QString& topPropertyName)
    {
        auto properties = pyObject.getVariable("properties");
        auto className = pyObject.getVariable("__class__");

        fillPropertiesFromPyObject(propertyManager, topProperty, properties,
                                   PythonQtObjectPtr{PythonQt::self()->callAndReturnPyObject(
                                       PythonQt::self()->lookupCallable(pyObject, "getProperties"))},
                                   className.toString());
        topProperty->setPropertyName(topPropertyName);
    }

    static void fillPropertiesFromPyObject(QtVariantPropertyManager* propertyManager, QtProperty* currentPropGroup,
                                           QVariant currentProperty, PythonQtObjectPtr accessor, const QString& className)
    {
        if (currentProperty.canConvert<QVariantMap>()) {
            // Single property
            auto valueMap = currentProperty.toMap();

            auto propertyAttributes = QVariantMap{};
            if (valueMap.contains("attributes")) {
                propertyAttributes = valueMap["attributes"].toMap();
                valueMap.remove("attributes");
            }

            auto propertyName = QString{};
            auto propertyValue = QVariant{};

            if (valueMap.contains("name")) {
                // Support for old version of property definition
                // i.e. {"name": "Pressure", "value": 100.0}
                propertyName = valueMap["name"].toString();

                if (valueMap.contains("value")) {
                    propertyValue = valueMap["value"];
                }
            } else {
                // Support for new version of property definition
                // i.e. {"Pressure": 100.0}
                if (!valueMap.empty()) {
                    propertyName = valueMap.begin().key();
                    propertyValue = valueMap.begin().value();
                }
            }

            auto newQtProperty = static_cast<QtVariantProperty*>(nullptr);

            auto propertyType = propertyValue.type();
            switch (propertyType) {
            case QVariant::Int:
                if (propertyAttributes.contains("enumNames")) {
                    propertyType = static_cast<QVariant::Type>(QtVariantPropertyManager::enumTypeId());
                }
                // Fall through
            case QVariant::Double:
            case QVariant::String:
            case QVariant::Bool:
                newQtProperty = propertyManager->addProperty(propertyType);
                QObject::connect(propertyManager, &QtVariantPropertyManager::valueChanged,
                                 [newQtProperty, accessor](QtProperty* prop, const QVariant& val) mutable {
                                     if (prop == newQtProperty) {
                                         accessor.call("__setitem__", QVariantList() << prop->propertyName() << val);
                                     }
                                 });
                break;
            case QVariant::List:
                newQtProperty = propertyManager->addProperty(QtVariantPropertyManager::groupTypeId());
                fillPropertiesFromPyObject(
                    propertyManager, newQtProperty, propertyValue,
                    PythonQtObjectPtr{PythonQt::self()->callAndReturnPyObject(
                        PythonQt::self()->lookupCallable(accessor, "__getitem__"), QVariantList() << propertyName)},
                    className);
                break;
            default:
                MITK_ERROR << "Unknown property type for property " << propertyName.toStdString() << " in "
                           << className.toStdString();
                break;
            }

            if (newQtProperty) {
                newQtProperty->setPropertyName(propertyName);
                for (auto& attributeNameValuePair : propertyAttributes.toStdMap()) {
                    newQtProperty->setAttribute(attributeNameValuePair.first, attributeNameValuePair.second);
                }

                if (propertyValue.type() == QVariant::Double && !propertyAttributes.contains("decimals")) {
                    newQtProperty->setAttribute("decimals", 50);
                }
                newQtProperty->setValue(propertyValue);

                currentPropGroup->addSubProperty(newQtProperty);
            }
        }

        if (currentProperty.canConvert<QVariantList>()) {
            // List of properties
            int i = 0;
            for (const auto& property : currentProperty.toList()) {
                fillPropertiesFromPyObject(propertyManager, currentPropGroup, property, accessor, className);
            }
        }
    }
};
}