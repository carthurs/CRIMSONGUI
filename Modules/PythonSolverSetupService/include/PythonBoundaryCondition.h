#pragma once

#include <set>

#include <IBoundaryCondition.h>
#include <PythonQtObjectPtr.h>

#include <mitkCommon.h>

#include "PythonFaceDataImpl.h"

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonBoundaryCondition : public IBoundaryCondition
{
public:
    mitkClassMacro(PythonBoundaryCondition, IBoundaryCondition);
    mitkNewMacro1Param(Self, PythonQtObjectPtr);
	//mitkNewMacro2Param(Self, PythonQtObjectPtr, mitk::BaseData::Pointer);
    //     itkCloneMacro(Self);
    //     mitkCloneMacro(Self);

    gsl::cstring_span<> getName() const override { return _impl.getName(); }
    bool isUnique() override;

    ImmutableRefRange<FaceIdentifier::FaceType> applicableFaceTypes() override { return _impl.applicableFaceTypes(); }
    void setFaces(ImmutableRefRange<FaceIdentifier> faces) override { _impl.setFaces(faces); }
    ImmutableRefRange<FaceIdentifier> getFaces() const override { return _impl.getFaces(); }
    gsl::not_null<QtPropertyStorage*> getPropertyStorage() override { return _impl.getPropertyStorage(); }
    QWidget* createCustomEditorWidget() override { return _impl.createCustomEditorWidget(); }

	void setDataObject(QVariantList data) override { _impl.setDataObject(data); } //sets a pointer to _dataObject in the python BC object

    PythonQtObjectPtr getPythonObject() const { return _impl.getPythonObject(); }

	std::string getDataUID(){ return _impl.getDataUID(); };

	void setDataUID(std::string dataUID) override { _impl.setDataUID(dataUID); }; 

protected:
    PythonBoundaryCondition(PythonQtObjectPtr pyBCObject);

	//PythonBoundaryCondition(PythonQtObjectPtr pyBCObject, mitk::BaseData::Pointer data);

    virtual ~PythonBoundaryCondition() {}

    PythonBoundaryCondition(const Self& /*other*/) = delete;

    PythonFaceDataImpl _impl;

	std::string _dataObjectUID;
};

} // end namespace crimson
