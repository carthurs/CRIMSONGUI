#include <PythonQt.h>

#include <mitkPythonService.h>

#include <PythonSolverSetupManager.h>
#include <PythonSolverParametersData.h>
#include <PythonSolverStudyData.h>
#include <PythonBoundaryConditionSet.h>
#include <PythonBoundaryCondition.h>
#include <PythonMaterialData.h>

#include <SolverSetupUtils.h>

#include "PythonQtWrappers.h"

#include "PythonSolverSetupServiceActivator.h"

namespace crimson
{

PythonSolverSetupManager::PythonSolverSetupManager(PythonQtObjectPtr pySolverSetupManagerObject, gsl::cstring_span<> name)
    : _pySolverSetupManagerObject(pySolverSetupManagerObject)
    , _name(gsl::to_string(name))
    , _boundaryConditionSetNames(_getClassNames("getBoundaryConditionSetNames"))
    , _solverParametersNames(_getClassNames("getSolverParametersNames"))
    , _solverStudyNames(_getClassNames("getSolverStudyNames"))
    , _materialNames(_getClassNames("getMaterialNames"))
{
    Expects(!pySolverSetupManagerObject.isNull());
}

template <typename... Args>
std::vector<std::string> PythonSolverSetupManager::_getClassNames(gsl::cstring_span<> functionName, Args&&... args) const
{
    auto resultVariant =
        _pySolverSetupManagerObject.call(gsl::to_QString(functionName), QVariantList{QVariant::fromValue(args)...}).toList();

    std::vector<std::string> result;
    std::transform(resultVariant.begin(), resultVariant.end(), std::back_inserter(result),
                   [](const QVariant& name) { return name.toString().toStdString(); });

    return result;
}

template <class T, typename... Args>
typename T::Pointer PythonSolverSetupManager::_createSolverObject(gsl::cstring_span<> functionName, Args&&... args)
{
    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

    auto pyBCObject =
        _pySolverSetupManagerObject.call(gsl::to_QString(functionName), QVariantList{QVariant::fromValue(args)...});

    if (pythonService->PythonErrorOccured() || !pyBCObject.canConvert<PythonQtObjectPtr>()) {
        return typename T::Pointer();
    }

    return T::New(qvariant_cast<PythonQtObjectPtr>(pyBCObject)).GetPointer();
}

template <class T, typename... Args>
typename T::Pointer PythonSolverSetupManager::_createSolverObjectWithCObject(gsl::cstring_span<> functionName, mitk::BaseData::Pointer data, Args&&... args)
{
	auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

	auto pyBCObject =
		_pySolverSetupManagerObject.call(gsl::to_QString(functionName), QVariantList{ QVariant::fromValue(args)... });

	if (pythonService->PythonErrorOccured() || !pyBCObject.canConvert<PythonQtObjectPtr>()) {
		return typename T::Pointer();
	}

	return T::New(qvariant_cast<PythonQtObjectPtr>(pyBCObject),data.GetPointer()).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getSolverStudyNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_solverStudyNames);
}

ISolverStudyData::Pointer PythonSolverSetupManager::createSolverStudy(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonSolverStudyData>("createSolverStudy", gsl::to_QString(name)).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getSolverParametersNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_solverParametersNames);
}

ISolverParametersData::Pointer PythonSolverSetupManager::createSolverParameters(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonSolverParametersData>("createSolverParameters", gsl::to_QString(name)).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getBoundaryConditionSetNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_boundaryConditionSetNames);
}

IBoundaryConditionSet::Pointer PythonSolverSetupManager::createBoundaryConditionSet(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonBoundaryConditionSet>("createBoundaryConditionSet", gsl::to_QString(name)).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getMaterialNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_materialNames);
}

IMaterialData::Pointer PythonSolverSetupManager::createMaterial(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonMaterialData>("createMaterial", gsl::to_QString(name)).GetPointer();
}


ImmutableValueRange<gsl::cstring_span<>>
PythonSolverSetupManager::getBoundaryConditionNameList(gsl::not_null<IBoundaryConditionSet*> ownerBCSet) const
{
    auto bcNamesIter = _boundaryConditionNames.find(ownerBCSet);
    if (bcNamesIter == _boundaryConditionNames.end()) {
        auto pythonOwnerBCSet = dynamic_cast<PythonBoundaryConditionSet*>(ownerBCSet.get());
        Expects(pythonOwnerBCSet != nullptr);

        bcNamesIter =
            _boundaryConditionNames.emplace(ownerBCSet.get(), std::move(_getClassNames("getBoundaryConditionNames",
                                                                                       pythonOwnerBCSet->getPythonObject())))
                .first;
    }

    return make_transforming_immutable_range<gsl::cstring_span<>>(bcNamesIter->second);
}

IBoundaryCondition::Pointer PythonSolverSetupManager::createBoundaryCondition(gsl::not_null<IBoundaryConditionSet*> ownerBCSet,
                                                                              gsl::cstring_span<> name)
{
    auto pythonOwnerBCSet = dynamic_cast<PythonBoundaryConditionSet*>(ownerBCSet.get());
    Expects(pythonOwnerBCSet != nullptr);

	//IBoundaryCondition::Pointer test = dynamic_cast<IBoundaryCondition*>(_createSolverObject<PythonBoundaryCondition>("createBoundaryCondition", gsl::to_QString(name),
	//	pythonOwnerBCSet->getPythonObject())
	//	.GetPointer());

	//auto pcmri2 = crimson::PCMRIData::New();
	//auto wrappedPCMRI2 = PCMRIDataQtWrapper{ pcmri2.GetPointer() };

	//auto pythBC = dynamic_cast<PythonBoundaryCondition*>(test.GetPointer());
	//Expects(pythBC != nullptr);

	//auto test2 = pythBC->getPythonObject();

	//test->setDataObject(QVariantList{ QVariant::fromValue(wrappedPCMRI2) });
	
	//if (gsl::to_QString(name) == "PCMRI")
	//{
	//	//itk::SmartPointer<crimson::PCMRIData> pcmri = (itk::SmartPointer<crimson::PCMRIData>) PCMRIData::New();

	//	auto pcmri = crimson::PCMRIData::New();

	//	//pcmriNode->SetData(pcmri);
	//	//pcmriNode->SetName("pcmriBC");
	//	//pcmriNode->SetProperty("hidden object", true);

	//	//std::unordered_set<mitk::DataNode*> potentialParents = HierarchyManager::getInstance()->findPotentialParents(pcmriNode, SolverSetupNodeTypes::PCMRIData());
	//	//crimson::HierarchyManager::getInstance()->addNodeToHierarchy(pcmriNode, SolverSetupNodeTypes::PCMRIData(),
	//	//	potentialParents.begin(), HierarchyManager::getInstance()->findFittingNodeType(potentialParents.begin()));

	//	IBoundaryCondition::Pointer BC = dynamic_cast<IBoundaryCondition*>(_createSolverObjectWithCObject<PythonBoundaryCondition>("createBoundaryCondition", pcmri.GetPointer(), gsl::to_QString(name),
	//		pythonOwnerBCSet->getPythonObject())
	//		.GetPointer());
	//	
	//	//dynamic_cast<PythonBoundaryCondition*>(BC)->setDataObject(pcmri.GetPointer());
	//	//auto wrappedPCMRI = PCMRIDataQtWrapper{ dynamic_cast<const PCMRIData*> (pcmri) };
	//	
	//	
	//	//auto wrappedPCMRI = PCMRIDataQtWrapper{ pcmri.GetPointer() };

	//	//TODO: call this inside MapAction after creating a pcmri map? and conenct the BC data object pointer to pcmri data object?
	//	//BC->setDataObject(QVariantList{ QVariant::fromValue(wrappedPCMRI) });

	//	//std::string uid;
	//	//pcmriNode->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, uid);
	//	//BC.get()->setObjectNodeUID(uid);

	//	//TODO: if name = PCMRI, make new PCMRIData node, assign PCMRIData reference to newly created python object (call a function that sets the PCMRIData)
	//	//QVariant::fromValue(MeshDataQtWrapper{meshData})
	//	//save by nodeUID, get the object (QtWrapped) only by calling a get() method upon writting PCMRI BC in writeSolverSetup()
	//	//void PythonSolverStudyData::setMeshNodeUID(gsl::cstring_span<> nodeUID) { _setStudyPartNodeUIDs("setMeshNodeUID", QVariantList{gsl::to_QString(nodeUID)}); }
	//	/*void SolverSetupView::setMeshNodeForStudy(const mitk::DataNode* node)
	//	{
	//	_updateUI();
	//	if (!node || !_currentSolverStudyNode) {
	//	return;
	//	}
	//	std::string uid;
	//	node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, uid);
	//	static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData())->setMeshNodeUID(uid);

	//	return T::New(qvariant_cast<PythonQtObjectPtr>(pyBCObject)).GetPointer();
	//	}*/
	//	return BC;
	//}
	//else
	//{
		IBoundaryCondition::Pointer BC = dynamic_cast<IBoundaryCondition*>(_createSolverObject<PythonBoundaryCondition>("createBoundaryCondition", gsl::to_QString(name),
			pythonOwnerBCSet->getPythonObject())
			.GetPointer());
		return BC;
	//}
	

}
}
