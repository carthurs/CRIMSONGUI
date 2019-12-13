#pragma once

#include <set>

#include <IMaterialData.h>
#include <PythonQtObjectPtr.h>

#include <mitkCommon.h>

#include "PythonFaceDataImpl.h"

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonMaterialData : public IMaterialData
{
public:
    mitkClassMacro(PythonMaterialData, IMaterialData);
    mitkNewMacro1Param(Self, PythonQtObjectPtr);
    //     itkCloneMacro(Self);
    //     mitkCloneMacro(Self);

    gsl::cstring_span<> getName() const override { return _impl.getName(); }

    ImmutableRefRange<FaceIdentifier::FaceType> applicableFaceTypes() override { return _impl.applicableFaceTypes(); }
    void setFaces(ImmutableRefRange<FaceIdentifier> faces) override { _impl.setFaces(faces); }
    ImmutableRefRange<FaceIdentifier> getFaces() const override { return _impl.getFaces(); }
    gsl::not_null<QtPropertyStorage*> getPropertyStorage() override { return _impl.getPropertyStorage(); }
    QWidget* createCustomEditorWidget() override { return _impl.createCustomEditorWidget(); }

    PythonQtObjectPtr getPythonObject() const { return _impl.getPythonObject(); }

protected:
    PythonMaterialData(PythonQtObjectPtr pyMaterialObject) : _impl(pyMaterialObject) {}
    virtual ~PythonMaterialData() {}

    PythonMaterialData(const Self& /*other*/) = delete;

    PythonFaceDataImpl _impl;
};

} // end namespace crimson
