#include "VesselForestData.h"
#include "VesselForestDataGraphTraits.h"

#include <algorithm>

#include <boost/graph/connected_components.hpp>

namespace crimson {

const VesselForestData::VesselPathUIDType VesselForestData::InflowUID{"inflow"};
const VesselForestData::VesselPathUIDType VesselForestData::OutflowUID{"outflow"};
const char* VesselForestData::topologyMTimePropertyName = "topologyMTime";

VesselForestData::VesselForestData() { VesselForestData::InitializeTimeGeometry(1); }

VesselForestData::VesselForestData(const Self& other)
    : mitk::BaseData(other)
    , _vesselUIDs(other._vesselUIDs)
    , _booleanOperations(other._booleanOperations)
{
    VesselForestData::InitializeTimeGeometry(1);
}

auto VesselForestData::getActiveVessels() const -> ImmutableRefRange<VesselPathUIDContainerType::value_type>
{
    return _vesselUIDs | boost::adaptors::filtered(ActiveVesselsFilter{this});
}

void VesselForestData::insertVessel(VesselPathAbstractData::Pointer vessel)
{
    const VesselPathUIDType& vesselUID = vessel->getVesselUID();

    if (_vesselUIDs.insert(vesselUID).second) {
        _vesselUsedInBlending[vesselUID] = true;
        vesselInsertedSignal(vesselUID);
        this->Modified();
    }
}

void VesselForestData::removeVessel(VesselPathAbstractData::Pointer vessel)
{
    const VesselPathUIDType& vesselUID = vessel->getVesselUID();

    if (_vesselUIDs.erase(vesselUID) == 0) {
        return;
    }

    // Remove the fillet information containing the removed vessel
    for (auto iter = _booleanOperations.begin(); iter != _booleanOperations.end();) {
        if (iter->vessels.first == vesselUID || iter->vessels.second == vesselUID) {
            iter = _booleanOperations.erase(iter);
        } else {
            ++iter;
        }
    }

    for (auto iter = _filletSizes.begin(); iter != _filletSizes.end();) {
        if (iter->first.first == vesselUID || iter->first.second == vesselUID) {
            iter = _filletSizes.erase(iter);
        } else {
            ++iter;
        }
    }

    vesselRemovedSignal(vesselUID);
    this->Modified();
}

void VesselForestData::UpdateOutputInformation()
{
    if (this->GetSource()) {
        this->GetSource()->UpdateOutputInformation();
    }
}

auto VesselForestData::getActiveBooleanOperations() const -> ImmutableRefRange<BooleanOperationContainerType::value_type>
{
    return _booleanOperations | boost::adaptors::filtered(ActiveBooleanOperationsFilter{this});
}

bool VesselForestData::addBooleanOperationInfo(const BooleanOperationInfo& info, int index)
{
    if (_vesselUIDs.count(info.vessels.first) == 0) {
        return false;
    }

    if (_vesselUIDs.count(info.vessels.second) == 0 && info.vessels.second != InflowUID && info.vessels.second != OutflowUID) {
        return false;
    }

    _booleanOperations.insert((index < 0 || index >= _booleanOperations.size()) ? _booleanOperations.end()
                                                                                : _booleanOperations.begin() + index,
                              info);

    booleanOperationAddedSignal(info);
    this->_topologyModified();
    this->Modified();

    return true;
}

bool VesselForestData::removeBooleanOperationInfo(const VesselPathUIDPair& vessels)
{
    // Find bop info
    auto iter = _findBooleanOperationInfo(vessels);

    if (iter == _booleanOperations.end()) {
        return false;
    }

    auto info = *iter;
    _booleanOperations.erase(iter);

    booleanOperationRemovedSignal(info);
    this->_topologyModified();
    this->Modified();

    return true;
}

bool VesselForestData::replaceBooleanOperationInfo(const BooleanOperationInfo& info)
{
    // Find bop info
    auto iter = _findBooleanOperationInfo(info.vessels);

    if (iter == _booleanOperations.end()) {
        return addBooleanOperationInfo(info);
    }

    *iter = info;
    booleanOperationChangedSignal(info);
    this->Modified();
    return true;
}

VesselForestData::BooleanOperationInfo VesselForestData::findBooleanOperationInfo(const VesselPathUIDPair& v) const
{
    auto iter = _findBooleanOperationInfo(v);

    return iter == _booleanOperations.end() ? BooleanOperationInfo{} : *iter;
}

auto VesselForestData::_findBooleanOperationInfo(const VesselPathUIDPair& v) const
    -> BooleanOperationContainerType::const_iterator
{
    return std::find_if(_booleanOperations.begin(), _booleanOperations.end(), [v](const BooleanOperationInfo& l) {
        return detail::OrderIndependentVesselPairEqualTo{}(l.vessels, v);
    });
}

auto VesselForestData::_findBooleanOperationInfo(const VesselPathUIDPair& v) -> BooleanOperationContainerType::iterator
{
    return std::find_if(_booleanOperations.begin(), _booleanOperations.end(), [v](const BooleanOperationInfo& l) {
        return detail::OrderIndependentVesselPairEqualTo{}(l.vessels, v);
    });
}

bool VesselForestData::swapBooleanOperations(const VesselPathUIDPair& from, const VesselPathUIDPair& to)
{
    auto iterFrom = _findBooleanOperationInfo(from);
    auto iterTo = _findBooleanOperationInfo(to);

    if (iterFrom == _booleanOperations.end() || iterTo == _booleanOperations.end()) {
        return false;
    }

    std::iter_swap(iterFrom, iterTo);
    booleanOperationsSwappedSignal(*iterFrom, *iterTo);
    this->Modified();
    return true;
}

auto VesselForestData::getActiveFilletSizeInfos() const -> ImmutableRefRange<FilletSizeInfoContainerType::value_type>
{
    return _filletSizes | boost::adaptors::filtered([this](const FilletSizeInfoContainerType::value_type& info) {
               return getVesselUsedInBlending(info.first.first) &&
                      (info.first.second == OutflowUID || info.first.second == InflowUID ||
                       getVesselUsedInBlending(info.first.second));
           });
}

bool VesselForestData::setFilletSizeInfo(const VesselPathUIDPair& vessels, double filletSize)
{
    if (_vesselUIDs.count(vessels.first) == 0 ||
        (_vesselUIDs.count(vessels.second) == 0 && vessels.second != InflowUID && vessels.second != OutflowUID)) {
        return false;
    }

    _filletSizes[vessels] = filletSize;
    filletChangedSignal(vessels, filletSize);
    this->Modified();
    return true;
}

bool VesselForestData::removeFilletSizeInfo(const VesselPathUIDPair& vessels)
{
    if (_filletSizes.erase(vessels) == 0) {
        return false;
    }

    filletRemovedSignal(vessels);
    this->Modified();
    return true;
}

auto VesselForestData::computeActiveConnectedComponents() const -> VesselPathUIDToConnectedComponentMap
{
    auto connectedComponentsMap = VesselPathUIDToConnectedComponentMap{};
    auto colorMap = std::map<std::string, boost::default_color_type>{};

    boost::connected_components(*this, boost::make_assoc_property_map(connectedComponentsMap),
                                boost::color_map(boost::make_assoc_property_map(colorMap)));

    return connectedComponentsMap;
}

bool VesselForestData::setVesselUsedInBlending(const VesselPathAbstractData* vessel, bool use)
{
    return setVesselUsedInBlending(vessel->getVesselUID(), use);
}

bool VesselForestData::setVesselUsedInBlending(const VesselPathUIDType& vesselUID, bool use)
{
    if (std::find(_vesselUIDs.begin(), _vesselUIDs.end(), vesselUID) == _vesselUIDs.end()) {
        return false;
    }

    if (_vesselUsedInBlending[vesselUID] != use) {
        _vesselUsedInBlending[vesselUID] = use;
        _topologyModified();
        this->Modified();
    }

    return true;
}

bool VesselForestData::getVesselUsedInBlending(const VesselPathAbstractData* vessel) const
{
    return getVesselUsedInBlending(vessel->getVesselUID());
}

int VesselForestData::getTopologyMTime() const
{
    int mTime = 0;
    GetPropertyList()->GetIntProperty(topologyMTimePropertyName, mTime);
    return mTime;
}

void VesselForestData::_topologyModified()
{
    GetPropertyList()->SetIntProperty(topologyMTimePropertyName, getTopologyMTime() + 1);
}

bool VesselForestData::getVesselUsedInBlending(const VesselPathUIDType& vesselUID) const
{
    auto iter = _vesselUsedInBlending.find(vesselUID);

    return (iter == _vesselUsedInBlending.end()) ? false : iter->second;
}

void VesselForestData::ExecuteOperation(mitk::Operation* operation)
{
    auto executedSuccessfully = [this, operation]() {
        if (ExecuteFilletOperation(dynamic_cast<FilletChangeOperation*>(operation))) {
            return true;
        }
        if (ExecuteBooleanInfoOperation(dynamic_cast<BooleanInfoChangeOperation*>(operation))) {
            return true;
        }
        return false;
    }();

    if (!executedSuccessfully) {
        MITK_WARN << "VesselForestData received wrong type of operation!";
        return;
    }

    mitk::OperationEndEvent endevent(operation);
    static_cast<const itk::Object*>(this)->InvokeEvent(endevent);
}

bool VesselForestData::ExecuteFilletOperation(FilletChangeOperation* filletSizeChangeOperation)
{
    if (!filletSizeChangeOperation) {
        return false;
    }

    switch (filletSizeChangeOperation->GetOperationType()) {
    case FilletChangeOperation::operationChange:
        return setFilletSizeInfo(filletSizeChangeOperation->GetVessels(), filletSizeChangeOperation->GetFilletSize());
    case FilletChangeOperation::operationRemove:
        return removeFilletSizeInfo(filletSizeChangeOperation->GetVessels());
    default:
        return false;
    }
}

bool VesselForestData::ExecuteBooleanInfoOperation(BooleanInfoChangeOperation* booleanInfoChangeOperation)
{
    if (!booleanInfoChangeOperation) {
        return false;
    }

    switch (booleanInfoChangeOperation->GetOperationType()) {
    case BooleanInfoChangeOperation::operationAdd:
        return addBooleanOperationInfo(booleanInfoChangeOperation->GetBooleanOperation(),
                                       booleanInfoChangeOperation->GetIndex());
    case BooleanInfoChangeOperation::operationRemove:
        return removeBooleanOperationInfo(booleanInfoChangeOperation->GetBooleanOperation().vessels);
    case BooleanInfoChangeOperation::operationChange:
        return replaceBooleanOperationInfo(booleanInfoChangeOperation->GetBooleanOperation());
    case BooleanInfoChangeOperation::operationSwap:
        return swapBooleanOperations(booleanInfoChangeOperation->GetBooleanOperation().vessels,
                                     booleanInfoChangeOperation->GetBooleanOperation2().vessels);
    default:
        return false;
    }
}
}
