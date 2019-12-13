#include "PythonFaceDataImpl.h"

#include "QtPropertyBrowserTools.h"

#include "PythonQtWrappers.h"
#include "SolverSetupPythonUtils.h"

namespace crimson
{

PythonFaceDataImpl::PythonFaceDataImpl(PythonQtObjectPtr pyObject)
    : _pyObject(pyObject)
{
    Expects(!pyObject.isNull());

    // Fill the properties
    auto name = QString{};
    auto humanReadableName = getClassVariable(_pyObject, "humanReadableName");

    if (!humanReadableName.isNull()) {
        name = humanReadableName.toString();
    } else {
        name = getClassVariable(_pyObject, "__name__").toString();
    }

    QtPropertyBrowserTools::fillPropertiesFromPyObject(_propertyStorage.getPropertyManager(), _propertyStorage.getTopProperty(),
                                                       _pyObject, name);

    _name = name.toStdString();
}

auto PythonFaceDataImpl::applicableFaceTypes() -> ImmutableRefRange<FaceIdentifier::FaceType>
{
    _applicableFaceTypes.clear();

    auto faceTypesVariantList = getClassVariable(_pyObject, "applicableFaceTypes");

    if (!faceTypesVariantList.isValid()) {
        MITK_ERROR << "Applicable face types not found for " << _name;
        return _applicableFaceTypes;
    }

    if (!faceTypesVariantList.canConvert<QVariantList>()) {
        MITK_ERROR << "Cannot convert applicable face types to list for " << _name;
        return _applicableFaceTypes;
    }

    for (const auto& faceTypeVariant : faceTypesVariantList.toList()) {
        if (!faceTypeVariant.canConvert<int>()) {
            MITK_WARN << "Cannot convert an applicable face type to integer for " << _name << ". Skipping.";
            continue;
        }

        auto faceTypeInt = faceTypeVariant.toInt();
        if (faceTypeInt < 0 || faceTypeInt >= FaceIdentifier::ftUndefined) {
            MITK_WARN << "Incorrect applicable face value" << faceTypeInt << " for " << _name
                      << ". Skipping.";
            continue;
        }

        _applicableFaceTypes.insert(static_cast<FaceIdentifier::FaceType>(faceTypeInt));
    }

    return _applicableFaceTypes;
}

void PythonFaceDataImpl::setFaces(ImmutableRefRange<FaceIdentifier> faces)
{
    QVariantList faceIdentifiersList;

    for (const auto& faceIdentifier : faces) {
        faceIdentifiersList.push_back(QVariant::fromValue(PyFaceIdentifier::cppToPy(faceIdentifier)));
    }

    _pyObject.call("setFaceIdentifiers", QVariantList{faceIdentifiersList});
}

auto PythonFaceDataImpl::getFaces() const -> ImmutableRefRange<FaceIdentifier>
{
    _faceIdCache.clear();

    QVariant faceIdentifiersVariantList = _pyObject.call("getFaceIdentifiers");

    if (!faceIdentifiersVariantList.isValid()) {
        MITK_ERROR << "Failed to call getFaceIdentifiers from " << _name;
        return _faceIdCache;
    }

    if (!faceIdentifiersVariantList.canConvert<QVariantList>()) {
        MITK_ERROR << "Cannot convert result of getFaceIdentifiers() to list for  " << _name;
        return _faceIdCache;
    }

    for (const auto& faceIdentifierVariant : faceIdentifiersVariantList.toList()) {
        auto faceIdentifierOptional = PyFaceIdentifier::pyToCpp(PythonQtObjectPtr{faceIdentifierVariant});

        if (!faceIdentifierOptional) {
            MITK_WARN << "Cannot convert a item of getFaceIdentifiers() to FaceIdentifier for " << _name
                      << ". Skipping.";
            continue;
        }
        _faceIdCache.insert(faceIdentifierOptional.get());
    }

    return _faceIdCache;
}

void PythonFaceDataImpl::setDataUID(std::string uid)
{
	_pyObject.call("setDataNodeUID", QVariantList{ QString::fromStdString(uid) });
}

std::string PythonFaceDataImpl::getDataUID() 
{
	std::string uid = std::string{};

	QVariant uidV = _pyObject.call("getDataNodeUID");

	if (!uidV.isValid()) {
		MITK_ERROR << "Failed to call uid from " << _name;
		return uid;
	}


	uid = uidV.toString().toStdString();

	return uid;
}


QWidget* PythonFaceDataImpl::createCustomEditorWidget()
{
    if (!_pyObject.getVariable("createCustomEditorWidget").isValid()) {
        return nullptr;
    }

    auto widgetVariant = _pyObject.call("createCustomEditorWidget");
    if (!widgetVariant.canConvert<QWidget*>()) {
        MITK_ERROR << "Failed to convert result of createCustomEditorWidget() to QWidget* from " << _name;
        return nullptr;
    }

    return qvariant_cast<QWidget*>(widgetVariant);
}

}
