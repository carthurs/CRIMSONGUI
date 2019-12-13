#pragma once

#include <set>
#include <string>
#include <map>
#include <vector>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/optional.hpp>

#include "SolidKernelExports.h"

namespace boost
{
namespace serialization
{
class access;
}
}
namespace crimson
{
class SolidData;
}

namespace crimson
{
/*!
 * \struct  FaceIdentifier
 *
 * \brief   A face identifier contains information on the list of vessels that were used to
 *  create the face. 
 *  
 *  Faces normally have only one parent solid index, but filleted faces have two
 *  or more. Multiple model faces might have the same face identifier if they are logically the
 *  same face and have only been created due to the model topology. Face identifiers allow to
 *  maintain the information associated with a logical face, e.g. boundary conditions, and
 *  persist the association through changes in the model, such as adding or removing vessels when
 *  the integer indices might change.
 */
struct SolidKernel_EXPORT FaceIdentifier {
    /*! \brief   Values that represent face types. */
    enum FaceType {
        ftCapInflow = 0,    ///< Inflow face
        ftCapOutflow,       ///< Outflow face
        ftWall,             ///< Wall face

        ftUndefined
    };

    FaceIdentifier()
        : faceType(ftUndefined)
    {
    }
    FaceIdentifier(const std::set<std::string>& parentSolidIndices, FaceType faceType)
        : parentSolidIndices(parentSolidIndices)
        , faceType(faceType)
    {
    }

    std::set<std::string> parentSolidIndices;   ///< List of vessel path UIDs that were used to create this face
    FaceType faceType;  ///< Type of the face

    bool operator==(const FaceIdentifier& r) const
    {
        return parentSolidIndices == r.parentSolidIndices && faceType == r.faceType;
    }

    bool operator<(const FaceIdentifier& r) const
    {
        if (parentSolidIndices == r.parentSolidIndices) {
            return faceType < r.faceType;
        }

        return std::lexicographical_compare(parentSolidIndices.begin(), parentSolidIndices.end(), r.parentSolidIndices.begin(),
                                            r.parentSolidIndices.end());
    }
};

/*!
 * \class   SolidKernel_EXPORT
 *
 * \brief   FaceIdentifierMap is used to map the logical FaceIdentifier to a set of physical
 *  model face indices.
 */
class SolidKernel_EXPORT FaceIdentifierMap 
{
public:

    /*!
     * \brief   Sets face identifier for model face.
     *
     * \param   modelFaceIndex  Zero-based index of the model face.
     * \param   id              The identifier.
     */
    void setFaceIdentifierForModelFace(int modelFaceIndex, const FaceIdentifier& id)
    {
        auto iter = std::find(_uniqueIdentifiers.begin(), _uniqueIdentifiers.end(), id);
        if (iter == _uniqueIdentifiers.end()) {
            _uniqueIdentifiers.push_back(id);
            _faceIdentifiers[modelFaceIndex] = static_cast<int>(_uniqueIdentifiers.size()) - 1;
        } else {
            _faceIdentifiers[modelFaceIndex] = static_cast<int>(std::distance(_uniqueIdentifiers.begin(), iter));
        }
    }

    /*!
     * \brief   Gets face identifier for model face.
     *
     * \param   modelFaceIndex  Zero-based index of the model face.
     *
     * \return  An optional face identifier for model face. Value is not set if modelFaceIndex is not found.
     */
    boost::optional<std::reference_wrapper<const FaceIdentifier>> getFaceIdentifierForModelFace(int modelFaceIndex) const
    {
        auto iter = _faceIdentifiers.find(modelFaceIndex);
        if (iter == _faceIdentifiers.end()) {
            return boost::none;
        }
        return std::cref(_uniqueIdentifiers[iter->second]);
    }

    /*!
     * \brief   Gets number of model faces.
     *
     * \return  The number of model faces.
     */
    int getNumberOfModelFaces() const { return static_cast<int>(_faceIdentifiers.size()); }

    /*!
     * \brief   Gets number of face identifiers.
     *
     * \return  The number of face identifiers.
     */
    int getNumberOfFaceIdentifiers() const { return static_cast<int>(_uniqueIdentifiers.size()); }

    /*!
     * \brief   Gets a zero-based index of a face identifier.
     *
     * \param   faceId  Face identifier.
     *
     * \return  Index in the list of unique face identifiers.
     */
    int faceIdentifierIndex(const FaceIdentifier& faceId) const
    {
        auto iter = std::find(_uniqueIdentifiers.begin(), _uniqueIdentifiers.end(), faceId);
        return iter == _uniqueIdentifiers.end() ? -1 : static_cast<int>(std::distance(_uniqueIdentifiers.begin(), iter));
    }

    /*!
     * \brief   Gets face identifier.
     *
     * \param   faceIdentifierIndex Zero-based index of the face identifier.
     *
     * \return  The face identifier.
     */
    const FaceIdentifier& getFaceIdentifier(int faceIdentifierIndex) const { return _uniqueIdentifiers[faceIdentifierIndex]; }

    /*!
     * \brief   Gets all the indices of the model faces assosciated with the face identifier.
     *
     * \param   id  The face identifier.
     *
     * \return  The model faces for face identifier.
     */
    std::vector<int> getModelFacesForFaceIdentifier(const FaceIdentifier& id) const
    {
        auto faceIdIndex = faceIdentifierIndex(id);
        if (faceIdIndex == -1) {
            return {};
        }

        auto out = std::vector<int>{};
        boost::push_back(out, _faceIdentifiers | boost::adaptors::filtered([faceIdIndex](const std::pair<const int, int>& kv) {
                                  return kv.second == faceIdIndex;
                              }) |
                                  boost::adaptors::map_keys);
        return out;
    }

private:
    template <class Archive> friend void serialize(Archive& ar, crimson::FaceIdentifierMap& data, const unsigned int version);
    template <class Archive> friend void serialize(Archive& ar, crimson::SolidData& data, const unsigned int version);
    friend class boost::serialization::access;

    std::map<int, int> _faceIdentifiers;
    std::vector<FaceIdentifier> _uniqueIdentifiers;
};
}