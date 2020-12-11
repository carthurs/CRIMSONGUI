#pragma once

#include <gsl.h>
#include <ImmutableRanges.h>
#include <qobject.h>

#include <mitkBaseData.h>
#include <FaceIdentifier.h>

class QWidget;

namespace crimson
{
class QtPropertyStorage;

/*! \brief   An interface for data associated with model faces.
 * 
 * This includes boundary conditions and materials.
 */
class FaceData : public mitk::BaseData
{
public:
    mitkClassMacro(FaceData, BaseData);

    /*!
     * \brief   Gets the FaceData object name.
     */
    virtual gsl::cstring_span<> getName() const = 0;

    /*!
     * \brief   Gets the list of face types (as defined by FaceIdentifer::FaceType) that the face
     *  data can be applied to.
     */
    virtual ImmutableRefRange<FaceIdentifier::FaceType> applicableFaceTypes() = 0;

    ///@{ 
    /*!
     * \brief   Sets the identifiers of the faces the FaceData is applied to.
     */
    virtual void setFaces(ImmutableRefRange<FaceIdentifier> faces) = 0;

    /*!
    * \brief   Gets the identifiers of the faces the FaceData is applied to.
    */
    virtual ImmutableRefRange<FaceIdentifier> getFaces() const = 0;
    ///@} 

    /*!
     * \brief   Creates custom editor widget which should be added to the UI.
     */
    virtual QWidget* createCustomEditorWidget() { return nullptr; }

    /*!
     * \brief   Gets property storage for the FaceData properties.
     */
    virtual gsl::not_null<QtPropertyStorage*> getPropertyStorage() = 0;

	/*!
	* \brief  stores a pointer to C++ owned data object. For use in python-based boundary conditions that have the logic
	implemented in C++
	*/
	virtual void setDataObject(QVariantList data){}; //TODO: this is very dirty since it implies python code. abstract it?
	//put wrapping down to FaceDataImpl and determine wrapper via if-else clause?

	virtual void setDataUID(std::string dataUID){};

	virtual std::string getDataUID(){ return std::string{}; };



protected:
    FaceData() { mitk::BaseData::InitializeTimeGeometry(1); }
    virtual ~FaceData() {}

    FaceData(const Self&) = default;

    void SetRequestedRegion(const itk::DataObject*) override {}
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return true; }
};

} // end namespace crimson
