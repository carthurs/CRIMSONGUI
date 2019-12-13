#pragma once

#include <vector>
#include <unordered_map>
#include <mitkBaseData.h>

#include "VesselPathAbstractData.h"
#include "VesselTreeExports.h"
#include <mitkOperation.h>

#include <boost/signals2.hpp>

#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/version.hpp>

#include <ImmutableRanges.h>

namespace crimson
{

class VesselForestDataIO;

namespace detail
{
using VesselPathUIDType = VesselPathAbstractData::VesselPathUIDType;

/*! \brief   Order-independent hash for vessel pairs. */
struct OrderIndependentVesselPairHasher {
    size_t operator()(const std::pair<VesselPathUIDType, VesselPathUIDType>& value) const
    {
        //
        return std::hash<size_t>()(std::hash<VesselPathUIDType>()(value.first) + std::hash<VesselPathUIDType>()(value.second));
    }
};

/*! \brief   Order-independent comparison for vessel pairs. */
struct OrderIndependentVesselPairEqualTo {
    using VesselPathUIDType = VesselPathAbstractData::VesselPathUIDType;

    bool operator()(const std::pair<VesselPathUIDType, VesselPathUIDType>& l,
                    const std::pair<VesselPathUIDType, VesselPathUIDType>& r) const
    {
        return (l.first == r.first && l.second == r.second) || (l.first == r.second && l.second == r.first);
    }
};
}

/*! \brief  Vessel tree data.
 *
 * VesselForestData contains the topology information of the vessel tree and the operations
 * involved in creating a blended model. This includes the information on the types and order
 * of boolean operations as well as fillet sizes to apply at the edges created after them.
 */
class VesselTree_EXPORT VesselForestData : public mitk::BaseData
{
public:
    using VesselPathUIDType = VesselPathAbstractData::VesselPathUIDType;       ///< Type of the vessel path UID
    using VesselPathUIDPair = std::pair<VesselPathUIDType, VesselPathUIDType>; ///< The vessel path UID pair

    /*! \brief   Values that represent boolean operation types. */
    enum BooleanOperationType {
        bopFuse,   ///< Fuse (union) operation
        bopCut,    ///< Cut (subtract) operation
        bopCommon, ///< Common (intersect) operation

        bopInvalidLast
    };

    /*! \brief   Information about the boolean operation. */
    struct BooleanOperationInfo {
        BooleanOperationInfo()
            : bop(bopInvalidLast)
            , removesFace(false)
        {
        }
        BooleanOperationInfo(const VesselPathUIDPair& vessels, BooleanOperationType bop, bool removesFace,
                             const VesselPathUIDType& removedFaceOwnerUID)
            : vessels(vessels)
            , bop(bop)
            , removesFace(removesFace)
            , removedFaceOwnerUID(removedFaceOwnerUID)
        {
        }

        VesselPathUIDPair vessels; ///< The vessels invoved in the boolean operation. 
        BooleanOperationType bop;  ///< The boolean operation type
        bool removesFace;          ///< Flag showing whether a whole face is removed after the operation is performed
        VesselPathUIDType removedFaceOwnerUID; ///< The vessel UID of the solid whose face was removed

        template <class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar& BOOST_SERIALIZATION_NVP(vessels);
            ar& BOOST_SERIALIZATION_NVP(bop);
            ar& BOOST_SERIALIZATION_NVP(removesFace);

            if (version > 0) {
                ar& BOOST_SERIALIZATION_NVP(removedFaceOwnerUID);
            }
        }
    };

    template <typename T>
    using OrderIndependentVesselPathMap = std::unordered_map<VesselPathUIDPair, T, detail::OrderIndependentVesselPairHasher,
                                                             detail::OrderIndependentVesselPairEqualTo>;

    using VesselPathUIDContainerType = std::set<VesselPathUIDType>;
    using BooleanOperationContainerType = std::vector<BooleanOperationInfo>;
    using FilletSizeInfoContainerType = OrderIndependentVesselPathMap<double>;
    using VesselPathUIDToConnectedComponentMap = std::map<VesselPathUIDType, int>;

public:
    mitkClassMacro(VesselForestData, BaseData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

    /*!
     * \brief   Gets all the vessels associated with this vessel tree.
     */
    const VesselPathUIDContainerType& getVessels() const { return _vesselUIDs; }

    /*!
     * \brief   Gets all the vessels which are not disabled by the used.
     */
    ImmutableRefRange<VesselPathUIDContainerType::value_type> getActiveVessels() const;

    /*!
     * \brief   Inserts a vessel to the vessel tree.
     */
    void insertVessel(VesselPathAbstractData::Pointer vessel);

    /*!
     * \brief   Removes the vessel for the vessel tree.
     */
    void removeVessel(VesselPathAbstractData::Pointer vessel);

    /*!
     * \brief   Gets all the boolean operations.
     */
    const BooleanOperationContainerType& getBooleanOperations() const { return _booleanOperations; }

    /*!
     * \brief   Gets only the boolean operations involving active vessels.
     */
    ImmutableRefRange<BooleanOperationContainerType::value_type> getActiveBooleanOperations() const;

    /*!
     * \brief   Adds a boolean operation information.
     *
     * \param   info    The BooleanOperationInfo instance.
     * \param   index   Position to insert the boolean operation to. Set to -1 (default value) to add to the end of the list.
     *
     * \return  false if any of the vessel UID's is not found in the vessel tree.
     */
    bool addBooleanOperationInfo(const BooleanOperationInfo& info, int index = -1);

    /*!
     * \brief   Replace boolean operation information. If the information to replace is not found, it will be added to the end
     * of the list.
     *
     * \param   info    The information to be replaced (defined by the vessel UID pair).
     *
     * \return  false if any of the vessel UID's is not found in the vessel tree.
     */
    bool replaceBooleanOperationInfo(const BooleanOperationInfo& info);

    /*!
     * \brief   Removes the boolean operation information for the vessel pair.
     *
     * \param   vessels The vessel UID pair.
     *
     * \return  false if the boolean operation information is not found.
     */
    bool removeBooleanOperationInfo(const VesselPathUIDPair& vessels);

    /*!
     * \brief   Finds the first boolean operation information for the vessel pair.
     *
     * \param   vessels   The vessel UID pair.
     *
     * \return  The found boolean operation information. If not found, the BooleanOperationInfo::bop is set to bopIvalidLast.
     */
    BooleanOperationInfo findBooleanOperationInfo(const VesselPathUIDPair& vessels) const;

    /*!
     * \brief   Swap the order of boolean operations.
     *
     * \param   from    First vessel UID pair.
     * \param   to      Second vessel UID pair.
     *
     * \return  false if any of the boolean operations is not found.
     */
    bool swapBooleanOperations(const VesselPathUIDPair& from, const VesselPathUIDPair& to);

    /*!
     * \brief   Gets all fillet sizes.
     *
     * \return  Order-independent map from vessel path UID pairs to fillet size.
     */
    const FilletSizeInfoContainerType& getFilletSizeInfos() const { return _filletSizes; }

    /*!
     * \brief   Gets only the fillet sizes involving active vessels.
     */
    ImmutableRefRange<FilletSizeInfoContainerType::value_type> getActiveFilletSizeInfos() const;

    /*!
     * \brief   Sets fillet size information.
     *
     * \param   vessels     The vessel UID pair. The second UID in the pair can also have
     *                      special values of VesselForestData::InflowUID or VesselForestData::OutflowUID
     *                      to signify a fillet at the edge between the wall and the inflow or outflow face
     * \param   filletSize  Size of the fillet.
     *
     * \return  false if any of the vessels is not found.
     */
    bool setFilletSizeInfo(const VesselPathUIDPair& vessels, double filletSize);

    /*!
     * \brief   Removes the fillet size information described by vessels.
     *
     * \param   vessels The vessel UID pair.
     *
     * \return  false if the fillet size information is not found.
     */
    bool removeFilletSizeInfo(const VesselPathUIDPair& vessels);

    /*!
     * \brief   Computes the connected components of the vessel tree graph between only the active vessels.
     *
     * \return  The calculated active connected components.
     */
    VesselPathUIDToConnectedComponentMap computeActiveConnectedComponents() const;

    static const VesselPathUIDType InflowUID;   ///< Special value for the second UID in a VesselUIDPair for the fillet at the inflow face
    static const VesselPathUIDType OutflowUID;  ///< Special value for the second UID in a VesselUIDPair for the fillet at the outflow face

    /*!
     * \brief   Used in blending flags - affects active vessels and the return values of all
     *  functions with 'Active' suffix.
     */
    bool setVesselUsedInBlending(const VesselPathUIDType& vessel, bool use);
    bool setVesselUsedInBlending(const VesselPathAbstractData* vessel, bool use);
    bool getVesselUsedInBlending(const VesselPathUIDType& vessel) const;
    bool getVesselUsedInBlending(const VesselPathAbstractData* vessel) const;

    /*!
     * \brief   Returns per-vessel-forest topology modification time.
     */
    int getTopologyMTime() const;

public:
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar& BOOST_SERIALIZATION_NVP(_vesselUIDs);
        ar& BOOST_SERIALIZATION_NVP(_vesselUsedInBlending);
        ar& BOOST_SERIALIZATION_NVP(_booleanOperations);
        ar& BOOST_SERIALIZATION_NVP(_filletSizes);
    }
    friend class boost::serialization::access;

public:
    void UpdateOutputInformation() override;
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return false; }
    bool VerifyRequestedRegion() override { return true; }
    void SetRequestedRegion(const itk::DataObject*) override {}
    void ExecuteOperation(mitk::Operation* operation) override;

public:
    boost::signals2::signal<void(const VesselPathUIDType&)> vesselInsertedSignal;   ///< The vessel inserted signal
    boost::signals2::signal<void(const VesselPathUIDType&)> vesselRemovedSignal;    ///< The vessel removed signal

    boost::signals2::signal<void(const VesselPathUIDPair&, double)> filletChangedSignal;    ///< The fillet changed signal
    boost::signals2::signal<void(const VesselPathUIDPair&)> filletRemovedSignal;    ///< The fillet removed signal

    boost::signals2::signal<void(const BooleanOperationInfo&)> booleanOperationAddedSignal; ///< The boolean operation added signal
    boost::signals2::signal<void(const BooleanOperationInfo&)> booleanOperationRemovedSignal;   ///< The boolean operation removed signal
    boost::signals2::signal<void(const BooleanOperationInfo&)> booleanOperationChangedSignal;   ///< The boolean operation changed signal
    boost::signals2::signal<void(const BooleanOperationInfo&, const BooleanOperationInfo&)> booleanOperationsSwappedSignal; ///< The boolean operations swapped signal

public:
    /*! \brief   A fillet change operation for undo-redo support. */
    class FilletChangeOperation : public mitk::Operation
    {
    public:
        enum FilletChangeOperationType {
            operationChange,
            operationRemove,
        };

        FilletChangeOperation(FilletChangeOperationType opType, const VesselPathUIDPair& vessels, float filletSize)
            : mitk::Operation(opType)
            , _vessels(vessels)
            , _filletSize(filletSize)
        {
        }

        VesselPathUIDPair GetVessels() const { return _vessels; }
        float GetFilletSize() const { return _filletSize; }

    private:
        VesselPathUIDPair _vessels;
        float _filletSize;
        void operator=(const Self&) = delete;
    };
    bool ExecuteFilletOperation(FilletChangeOperation* filletSizeChangeEvent);

    /*! \brief   A boolean information change operation for undo-redo support. */
    class BooleanInfoChangeOperation : public mitk::Operation
    {
    public:
        enum BooleanInfoChangeOperationType {
            operationAdd,
            operationRemove,
            operationChange,
            operationSwap,
            operationSwapArguments
        };

        BooleanInfoChangeOperation(BooleanInfoChangeOperationType opType, const BooleanOperationInfo& bop,
                                   const BooleanOperationInfo& bop2 = BooleanOperationInfo{}, int index = -1)
            : mitk::Operation(opType)
            , _booleanOperation(bop)
            , _booleanOperation2(bop2)
            , _index(index)
        {
        }

        int GetIndex() const { return _index; }
        BooleanOperationInfo GetBooleanOperation() const { return _booleanOperation; }
        BooleanOperationInfo GetBooleanOperation2() const { return _booleanOperation2; }

    private:
        BooleanOperationInfo _booleanOperation;
        BooleanOperationInfo _booleanOperation2;
        int _index;
        void operator=(const Self&) = delete;
    };
    bool ExecuteBooleanInfoOperation(BooleanInfoChangeOperation* booleanInfoChangeEvent);

protected:
    VesselForestData();

    VesselForestData(const Self& other);

    void _topologyModified();

private:
    VesselPathUIDContainerType _vesselUIDs;
    std::unordered_map<std::string, bool> _vesselUsedInBlending;
    BooleanOperationContainerType _booleanOperations; // Ordered boolean operations
    FilletSizeInfoContainerType _filletSizes;

    BooleanOperationContainerType::const_iterator _findBooleanOperationInfo(const VesselPathUIDPair& v) const;
    BooleanOperationContainerType::iterator _findBooleanOperationInfo(const VesselPathUIDPair& v);

    static const char* topologyMTimePropertyName;

    friend class VesselForestDataIO;
};
}

BOOST_CLASS_VERSION(crimson::VesselForestData::BooleanOperationInfo, 1)