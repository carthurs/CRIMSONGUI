#pragma once

#include <set>

#include <IBoundaryCondition.h>
#include <PythonQtObjectPtr.h>

#include <mitkCommon.h>

#include <QtPropertyStorage.h>

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonFaceDataImpl
{
public:
    PythonFaceDataImpl(PythonQtObjectPtr pyBCObject);
    virtual ~PythonFaceDataImpl() {}

    gsl::cstring_span<> getName() const { return _name; }

    ImmutableRefRange<FaceIdentifier::FaceType> applicableFaceTypes();
    void setFaces(ImmutableRefRange<FaceIdentifier> faces);
    ImmutableRefRange<FaceIdentifier> getFaces() const;
    gsl::not_null<QtPropertyStorage*> getPropertyStorage() { return &_propertyStorage; }
    QWidget* createCustomEditorWidget();

	void setDataObject(QVariantList& args){
		_pyObject.call("setDataObject", args);
	};

	void setDataUID(std::string uid);

	std::string getDataUID();

    PythonQtObjectPtr getPythonObject() const { return _pyObject; }

protected:
    PythonFaceDataImpl(const PythonFaceDataImpl& /*other*/) = delete;

    void _fillProperties(QtProperty* currentPropGroup, QVariant props, PythonQtObjectPtr accessor);

    mutable PythonQtObjectPtr _pyObject;
    std::string _name;

    std::set<FaceIdentifier::FaceType> _applicableFaceTypes;
    std::set<FaceIdentifier> _faces;
    QtPropertyStorage _propertyStorage;

    mutable std::set<FaceIdentifier> _faceIdCache;
};

} // end namespace crimson
