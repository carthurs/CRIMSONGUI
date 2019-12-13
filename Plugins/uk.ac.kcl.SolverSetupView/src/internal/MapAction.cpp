#include "MapAction.h"
#include "PCMRIUtils.h"
#include "PCMRIMappingWidget.h"

#include <HierarchyManager.h>
#include <SolverSetupNodeTypes.h>

#include <AsyncTaskManager.h>
#include <CreateDataNodeAsyncTask.h>

#include <IPCMRIKernel.h>
#include <MeshData.h>
#include <FaceData.h>
#include <SolidData.h>

#include <mitkProperties.h>
#include <mitkPlanarPolygon.h>
#include <mitkPlanarSubdivisionPolygon.h>
#include <mitkPointSet.h>
#include <mitkExtractSliceFilter.h>
#include <mitkVtkImageOverwrite.h>
#include <itkDiscreteGaussianImageFilter.h>
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "mitkImageTimeSelector.h"
#include "mitkImageSliceSelector.h"
#include "mitkImageCast.h"
#include "mitkITKImageImport.h"
#include <mitkMaskImageFilter.h>
#include <itkMaskImageFilter.h>
#include <itkDivideImageFilter.h>
#include "itkMultiplyImageFilter.h"
#include "itkSubtractImageFilter.h"
#include <mitkPlanarFigureMaskGenerator.h>

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkBinaryImageToLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"
#include "itkLabelStatisticsImageFilter.h"
#include "itkRegionOfInterestImageFilter.h"
#include <itkImageSliceConstIteratorWithIndex.h>
#include "itkPasteImageFilter.h"

#include <itkMetaDataObject.h>
#include "mitkImageAccessByItk.h"


#include <iostream>


typedef itk::Image<short, 2>                                        ShortSliceType;
typedef itk::Image<float, 2>                                        FloatSliceType;
typedef itk::Image<short, 4>                                        ShortImageType;
typedef itk::Image<float, 4>                                        FloatImageType;
typedef itk::Image<unsigned short, 2>                               UShortSliceType;
typedef itk::Image<unsigned short, 4>					            UShortImageType;
typedef itk::DiscreteGaussianImageFilter< FloatSliceType, FloatSliceType>     GaussianFilterType;
typedef itk::DivideImageFilter <FloatSliceType, FloatSliceType, FloatSliceType >   DivideImageFilterType;
typedef itk::MultiplyImageFilter <FloatSliceType, FloatSliceType >            MultiplyImageFilterType;
typedef itk::DivideImageFilter <FloatImageType, FloatImageType, FloatImageType >   DivideScalingType;
typedef itk::MultiplyImageFilter <FloatImageType, FloatImageType >            MultipyScalingType;
typedef itk::SubtractImageFilter <FloatImageType, FloatImageType, FloatImageType >   SubtractScalingType;


FloatImageType::Pointer gaussianSmooth(FloatImageType::Pointer filteredImage, mitk::DataNode::Pointer node, double profileSmoothness, std::vector<mitk::DataNode*> contourNodes)
{
	///Extractors
	typedef itk::ExtractImageFilter< FloatImageType, FloatSliceType > ExtractFilterType;
	typedef itk::PasteImageFilter< FloatImageType > PasteFilterType;

	///Setup for pasting
	PasteFilterType::Pointer pasteFilter = PasteFilterType::New();


	//check if it's a par/rec image
	int timeNr = 0;
	int numPhases = 0;
	if (dynamic_cast<mitk::Image*>(node->GetData())->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", numPhases))
	{
		//the image is par/rec
		timeNr = 2 * numPhases;
	}

	int contourIdx = 0;
	itk::Index<4> index;
	index[0] = 0;
	index[1] = 0;
	index[2] = 0;
	index[3] = timeNr;
	for (int i = 0; i < contourNodes.size(); i++)
	{
		mitk::ImageTimeSelector::Pointer timeSelector = mitk::ImageTimeSelector::New();
		mitk::ImageSliceSelector::Pointer sliceSelector = mitk::ImageSliceSelector::New();
		sliceSelector->SetInput(dynamic_cast<mitk::Image*>(node->GetData()));

		//check if it's a par/rec image
		int time = 0;
		if (numPhases)
		{
			//the image is par/rec
			time = 2 * numPhases + contourIdx;
		}
		else
			time = contourIdx;

		sliceSelector->SetTimeNr(time);
		sliceSelector->SetSliceNr(0);
		sliceSelector->Update();
		mitk::Image::Pointer newImage = sliceSelector->GetOutput();


		///Setup region of the slice to extract
		FloatImageType::IndexType sliceIndex;
		sliceIndex[0] = 0;
		sliceIndex[1] = 0;
		sliceIndex[2] = 0;
		sliceIndex[3] = time;
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = filteredImage->GetLargestPossibleRegion().GetSize();
		sliceSize[2] = 0;
		sliceSize[3] = 0;
		ExtractFilterType::InputImageRegionType sliceRegion;
		sliceRegion.SetSize(sliceSize);
		sliceRegion.SetIndex(sliceIndex);

		///Pull out slice
		ExtractFilterType::Pointer inExtractor = ExtractFilterType::New(); ///Must be within loop so that smart pointer is unique
		inExtractor->SetInput(filteredImage);
		inExtractor->SetExtractionRegion(sliceRegion);
		inExtractor->SetDirectionCollapseToIdentity();
		inExtractor->Update();

		mitk::DataNode::Pointer contourNode = contourNodes.at(contourIdx);

		//mask the image
		mitk::PlanarFigure::Pointer contour = dynamic_cast<mitk::PlanarFigure*>(contourNode->GetData());
		mitk::PlanarFigureMaskGenerator::Pointer maskGenerator = mitk::PlanarFigureMaskGenerator::New();
		maskGenerator->SetPlanarFigure(contour);
		maskGenerator->SetInputImage(newImage);
		mitk::Image::Pointer mask = maskGenerator->GetMask();


		ShortSliceType::Pointer itkMaskShort = ShortSliceType::New();
		CastToItkImage(mask, itkMaskShort);
		
		//cast binary mask from short to float to enable Gaussian smoothing
		typedef itk::CastImageFilter< ShortSliceType, FloatSliceType > FilterMaskType;
		FilterMaskType::Pointer filterMask = FilterMaskType::New();
		filterMask->SetInput(itkMaskShort);

		//mask the original image
		typedef itk::MaskImageFilter< FloatSliceType, FloatSliceType > MaskFilterType;
		MaskFilterType::Pointer maskFilter = MaskFilterType::New();
		maskFilter->SetInput(inExtractor->GetOutput());
		maskFilter->SetMaskImage(filterMask->GetOutput());
		maskFilter->SetDirectionTolerance(0.001);
		
		FloatSliceType::Pointer newImageMasked = maskFilter->GetOutput();


		GaussianFilterType::Pointer gaussianFilter = GaussianFilterType::New();
		gaussianFilter->SetInput(maskFilter->GetOutput());
		gaussianFilter->SetVariance(profileSmoothness);
		gaussianFilter->UpdateLargestPossibleRegion();
		gaussianFilter->SetUseImageSpacingOff();
		gaussianFilter->SetNumberOfThreads(8);
		gaussianFilter->Update();

		//filter the mask
		GaussianFilterType::Pointer gaussianFilterMask = GaussianFilterType::New();
		gaussianFilterMask->SetInput(filterMask->GetOutput());
		gaussianFilterMask->SetVariance(profileSmoothness);
		gaussianFilterMask->UpdateLargestPossibleRegion();
		gaussianFilterMask->SetUseImageSpacingOff();
		gaussianFilterMask->SetNumberOfThreads(8);
		gaussianFilterMask->Update();


		//divide filtered image with filtered mask
		DivideImageFilterType::Pointer divideImageFilter = DivideImageFilterType::New();
		divideImageFilter->SetInput1(gaussianFilter->GetOutput());
		divideImageFilter->SetInput2(gaussianFilterMask->GetOutput());
		divideImageFilter->Update();


		//make sure values which were divided by 0 are set back to 0
		MultiplyImageFilterType::Pointer multiplyFilter = MultiplyImageFilterType::New();
		multiplyFilter->SetInput1(divideImageFilter->GetOutput());
		multiplyFilter->SetInput2(filterMask->GetOutput());
		multiplyFilter->Update();

		//Get the mean value of intensities inside the masked region
		typedef itk::BinaryImageToLabelMapFilter<FloatSliceType> BinaryImageToLabelMapFilterType;
		BinaryImageToLabelMapFilterType::Pointer binaryImageToLabelMapFilter = BinaryImageToLabelMapFilterType::New();
		binaryImageToLabelMapFilter->SetInput(filterMask->GetOutput());
		binaryImageToLabelMapFilter->SetInputForegroundValue(1);
		binaryImageToLabelMapFilter->Update();

		typedef itk::LabelMapToLabelImageFilter<BinaryImageToLabelMapFilterType::OutputImageType, ShortSliceType> LabelMapToLabelImageFilterType;
		LabelMapToLabelImageFilterType::Pointer labelMapToLabelImageFilter = LabelMapToLabelImageFilterType::New();
		labelMapToLabelImageFilter->SetInput(binaryImageToLabelMapFilter->GetOutput());
		labelMapToLabelImageFilter->Update();

		typedef itk::LabelStatisticsImageFilter< FloatSliceType, ShortSliceType > LabelStatisticsImageFilterType;
		LabelStatisticsImageFilterType::Pointer labelStatisticsImageFilter = LabelStatisticsImageFilterType::New();
		labelStatisticsImageFilter->SetLabelInput(labelMapToLabelImageFilter->GetOutput());
		labelStatisticsImageFilter->SetInput(multiplyFilter->GetOutput());
		labelStatisticsImageFilter->Update();



		typedef LabelStatisticsImageFilterType::ValidLabelValuesContainerType ValidLabelValuesType;
		typedef LabelStatisticsImageFilterType::LabelPixelType                LabelPixelType;

		for (ValidLabelValuesType::const_iterator vIt = labelStatisticsImageFilter->GetValidLabelValues().begin();
			vIt != labelStatisticsImageFilter->GetValidLabelValues().end();
			++vIt)
		{
			if (labelStatisticsImageFilter->HasLabel(*vIt))
			{
				LabelPixelType labelValue = *vIt;

			}
		}

		maskFilter->SetInput(divideImageFilter->GetOutput());
		maskFilter->SetMaskImage(filterMask->GetOutput());
		maskFilter->SetOutsideValue(labelStatisticsImageFilter->GetMean(1));
		maskFilter->Update();


		typedef itk::CastImageFilter< FloatSliceType, FloatImageType > CastFilterType;
		CastFilterType::Pointer castFilter = CastFilterType::New();
		castFilter->SetInput(maskFilter->GetOutput());
		castFilter->Update();

		///Save Slice
		FloatImageType::SizeType sourceSize;
		sourceSize = filteredImage->GetLargestPossibleRegion().GetSize();
		sourceSize[2] = 1;
		sourceSize[3] = 1;
		FloatImageType::RegionType sourceRegion;
		FloatImageType::RegionType::IndexType index;
		index.Fill(0);
		sourceRegion.SetSize(sourceSize);
		sourceRegion.SetIndex(index);
		pasteFilter->SetSourceImage(castFilter->GetOutput());
		pasteFilter->SetDestinationImage(filteredImage);
		pasteFilter->SetDestinationIndex(sliceIndex);
		pasteFilter->SetSourceRegion(sourceRegion);
		pasteFilter->Update();

		filteredImage = pasteFilter->GetOutput();
		filteredImage->DisconnectPipeline();

		contourIdx++;
	}

	return filteredImage.GetPointer();
}

bool sortContours(mitk::DataNode* i, mitk::DataNode* j)
{
	float p1, p2;
	static_cast<mitk::DataNode*>(i)->GetFloatProperty("mapping.parameterValue", p1);
	static_cast<mitk::DataNode*>(j)->GetFloatProperty("mapping.parameterValue", p2);
	return (p1<p2);
}

double calculateParRecScaling(mitk::Image::Pointer pcmriImage, int startIndex)
{
	int phaseEncodingVelocityValue = 0;
	std::vector<short> phaseValues;
	double max_ph = 0;

	pcmriImage->GetPropertyList()->GetIntProperty("mapping.vencP", phaseEncodingVelocityValue);

	for (int i = startIndex; i < pcmriImage->GetDimensions()[3]; i++)
	{
		for (int j = 0; j < pcmriImage->GetDimensions()[0]; j++)
		{
			for (int z = 0; z < pcmriImage->GetDimensions()[1]; z++)
			{
				itk::Index<3> idx;
				idx[0] = j;
				idx[1] = z;
				idx[2] = 0;
				phaseValues.push_back(pcmriImage->GetPixelValueByIndex(idx, i));
				auto pixValue = pcmriImage->GetPixelValueByIndex(idx, i);
				if (pcmriImage->GetPixelValueByIndex(idx, i)>max_ph)
					max_ph = pcmriImage->GetPixelValueByIndex(idx, i);
			}
		}
	}

	int power = ceil(log2(max_ph));
	double scale = (pow(2, power)) / 2;

	return scale;
}

template <typename T>
FloatImageType::Pointer calculateVelocities(mitk::Image::Pointer pcmriImage, mitk::Image::Pointer magnitudeImage, mitk::PropertyList::Pointer properties)
{

	//cast mitk image to itk 
	using CastType = mitk::ImageToItk<itk::Image<T, 4>>;
	CastType::Pointer caster = CastType::New();

	using ImageType = itk::Image<T, 4>;
	ImageType::Pointer itkImage = ImageType::New();
	caster->SetInput(pcmriImage);
	caster->Update();
	itkImage = caster->GetOutput();

	//cast itk short to itk float
	typedef itk::CastImageFilter< ImageType, FloatImageType > CastToFloatType;
	CastToFloatType::Pointer castToFloat = CastToFloatType::New();
	FloatImageType::Pointer itkImageFloat = FloatImageType::New();
	castToFloat->SetInput(itkImage);
	castToFloat->Update();
	itkImageFloat = castToFloat->GetOutput();
	itkImageFloat->DisconnectPipeline();


	std::string velocityCalculationType;

	properties->GetStringProperty("mapping.velocityCalculationType", velocityCalculationType);

	if (velocityCalculationType == "Philips")
	{
		int numPhases = 0;
		int phaseEncodingVelocityValue = 0;
		int startIndex = 0;
		double scale = 0;

		properties->GetIntProperty("PAR.MaxNumberOfCardiacPhases", numPhases);
		properties->GetIntProperty("mapping.vencP", phaseEncodingVelocityValue);


		SubtractScalingType::Pointer subtractImageFilter = SubtractScalingType::New();
		MultipyScalingType::Pointer multiplyImageFilter = MultipyScalingType::New();

		startIndex = 2 * numPhases;
		scale = calculateParRecScaling(pcmriImage.GetPointer(), startIndex);

		subtractImageFilter->SetInput1(itkImageFloat);
		subtractImageFilter->SetConstant2(scale);
		subtractImageFilter->Update();

		multiplyImageFilter->SetInput(subtractImageFilter->GetOutput());
		multiplyImageFilter->SetConstant(phaseEncodingVelocityValue * 10 / scale); //multiply by 10 to convert from cm to mm
		multiplyImageFilter->Update();

		itkImageFloat = multiplyImageFilter->GetOutput();


	}
	else if (velocityCalculationType == "GE")
	{
		int venc = 0;
		double venscale = 0;
		bool isMagnitudeMask = 0;

		properties->GetDoubleProperty("mapping.venscale", venscale);
		properties->GetIntProperty("mapping.vencG", venc);
		properties->GetBoolProperty("mapping.magnitudemask", isMagnitudeMask);

		double scale = (venscale*M_PI) / (double)venc;

		DivideScalingType::Pointer divideImageFilter = DivideScalingType::New();
		MultipyScalingType::Pointer multiplyImageFilter = MultipyScalingType::New();

		if (isMagnitudeMask)
		{
			caster->SetInput(magnitudeImage);
			caster->Update();
			castToFloat->SetInput(caster->GetOutput());
			castToFloat->Update();

			multiplyImageFilter->SetInput(castToFloat->GetOutput());
			multiplyImageFilter->SetConstant(scale / 10); //divide by 10 to convert from cm to mm 
			multiplyImageFilter->Update();

			divideImageFilter->SetInput1(itkImageFloat.GetPointer());
			divideImageFilter->SetInput2(multiplyImageFilter->GetOutput());
			divideImageFilter->Update();

			itkImageFloat = divideImageFilter->GetOutput();

		}
		else
		{
			divideImageFilter->SetInput(itkImageFloat);
			divideImageFilter->SetConstant(scale/10); //multiply image by 10 to convert from cm to mm 
			divideImageFilter->Update();

			itkImageFloat = divideImageFilter->GetOutput();
		}

	}
	else if (velocityCalculationType == "Siemens")
	{
		int venc = 0;
		double rescaleSlope = 0;
		int rescaleIntercept = 0;
		int quantization = 0;

		properties->GetIntProperty("mapping.rescaleInterceptS", rescaleIntercept);
		properties->GetDoubleProperty("mapping.rescaleSlopeS", rescaleSlope);
		properties->GetIntProperty("mapping.vencS", venc);
		properties->GetIntProperty("mapping.quantization", quantization);

		double scale = venc * 10 / (pow(2, quantization)); //multiply by 10 to convert from cm to mm

		SubtractScalingType::Pointer subtractImageFilter = SubtractScalingType::New();
		MultipyScalingType::Pointer multiplyImageFilter = MultipyScalingType::New();

		multiplyImageFilter->SetInput(itkImageFloat);
		multiplyImageFilter->SetConstant(scale * rescaleSlope);
		multiplyImageFilter->Update();

		subtractImageFilter->SetInput1(multiplyImageFilter->GetOutput());
		subtractImageFilter->SetInput2((double)rescaleIntercept * scale * -1);
		subtractImageFilter->Update();

		itkImageFloat = subtractImageFilter->GetOutput();

	}

	else if (velocityCalculationType == "Linear")
	{
		double rescaleSlope = 0;
		int rescaleIntercept = 0;
		properties->GetIntProperty("mapping.rescaleIntercept", rescaleIntercept);
		properties->GetDoubleProperty("mapping.rescaleSlope", rescaleSlope);

		SubtractScalingType::Pointer subtractImageFilter = SubtractScalingType::New();
		MultipyScalingType::Pointer multiplyImageFilter = MultipyScalingType::New();
		MultipyScalingType::Pointer multiplyImageFilter2 = MultipyScalingType::New();

		multiplyImageFilter->SetInput(itkImageFloat);
		multiplyImageFilter->SetConstant(rescaleSlope);
		multiplyImageFilter->Update();

		subtractImageFilter->SetInput1(multiplyImageFilter->GetOutput());
		subtractImageFilter->SetConstant2((double)rescaleIntercept);
		subtractImageFilter->Update();

		multiplyImageFilter2->SetInput(subtractImageFilter->GetOutput());
		multiplyImageFilter2->SetConstant(10); //multiply by 10 to convert from cm to mm
		multiplyImageFilter2->Update();

		itkImageFloat = multiplyImageFilter2->GetOutput();

	}

	return itkImageFloat.GetPointer();
}

MapAction::MapAction()
{
}

MapAction::~MapAction() {}

std::shared_ptr<crimson::CreateDataNodeAsyncTask> MapAction::Run(const mitk::DataNode::Pointer& node)
{
	auto hm = crimson::HierarchyManager::getInstance();

	if (!hm->getPredicate(crimson::SolverSetupNodeTypes::BoundaryCondition())->CheckNode(node)) {
		return std::shared_ptr<crimson::CreateDataNodeAsyncTask>();
	}

	auto meshUID = std::string{};
	node->GetStringProperty("mapping.meshnode", meshUID);
	mitk::DataNode* meshNode = crimson::HierarchyManager::getInstance()->findNodeByUID(meshUID);

	auto pcmriUID = std::string{};
	node->GetStringProperty("mapping.pcmrinode", pcmriUID);
	mitk::DataNode* pcmriNodeMagnitude = crimson::HierarchyManager::getInstance()->findNodeByUID(pcmriUID);

	auto solidUID = std::string{};
	node->GetStringProperty("mapping.solidnode", solidUID);
	mitk::DataNode* solidNode = crimson::HierarchyManager::getInstance()->findNodeByUID(solidUID);

	auto phaseUID = std::string{};
	node->GetStringProperty("mapping.pcmrinodephase", phaseUID);
	mitk::DataNode* pcmriNodePhase = crimson::HierarchyManager::getInstance()->findNodeByUID(phaseUID);

	auto testProps = pcmriNodePhase->GetPropertyList();

	//get the PCMRI contour nodes

	mitk::Image::Pointer pcmriMagnitudeImage = dynamic_cast<mitk::Image*>(pcmriNodeMagnitude->GetData());

	int nSlices;
	if (!pcmriMagnitudeImage->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", nSlices))
		//if this field does not exist, it's a DICOM file
		nSlices = pcmriMagnitudeImage->GetDimensions()[3];

	
	std::vector<mitk::DataNode*> contourNodes = crimson::PCMRIUtils::getContourNodesSortedByParameter(pcmriNodeMagnitude);

	crimson::IPCMRIKernel::ContourSet contoursPCMRI(contourNodes.size());

	// Extract contours from the data nodes
	std::transform(contourNodes.begin(), contourNodes.end(), contoursPCMRI.begin(), [](const mitk::DataNode* n) {
		auto pf = static_cast<mitk::PlanarFigure*>(n->GetData());
		float p;
		n->GetFloatProperty("mapping.parameterValue", p);
		pf->GetPropertyList()->SetFloatProperty("mapping.parameterValue", p);
		return pf;
	});

	// Ignore unfinished contours
	contoursPCMRI.erase(std::remove_if(contoursPCMRI.begin(), contoursPCMRI.end(), [](const mitk::PlanarFigure* pf) {
		return !pf->IsFinalized();
	}), contoursPCMRI.end());


	//get the model face contour points
	auto mesh = static_cast<crimson::MeshData*>(meshNode->GetData());
	
	crimson::FaceData* fData = static_cast<crimson::FaceData*>(node->GetData());

	auto selectedFaces = fData->getFaces();
	crimson::FaceIdentifier face = *selectedFaces.begin();

	//TODO: check if reference image spacing here messes up mapping geometries
	auto plane = dynamic_cast<crimson::SolidData*>(solidNode->GetData())->getFaceGeometry(*selectedFaces.begin(), 1, 100);

	//get the model boundary points 

	std::vector<int> faceBoundaryNodes = mesh->getBoundaryNodeIdsForFace(face);

	std::vector<mitk::Point3D> contourModel3D;
	for (std::vector<int>::iterator it = faceBoundaryNodes.begin(); it < faceBoundaryNodes.end(); it++) {
		contourModel3D.push_back(mesh->getNodeCoordinates(*it));
	}

	//transform boundary model points to 2D

	std::vector<mitk::Point2D> contourModel2D;

	for (std::vector<mitk::Point3D>::iterator it = contourModel3D.begin(); it < contourModel3D.end(); it++) {
		mitk::Point2D projectedPoint;
		plane->Map(*it, projectedPoint);
		contourModel2D.push_back(projectedPoint);
	}

	//transfer vector of points to PlanarFigure

	mitk::PlanarFigure::Pointer figure = mitk::PlanarSubdivisionPolygon::New();
	//Set points to figure
	for (int i = 0; i < contourModel2D.size(); i++)
	{
		mitk::Point2D indexPoint2d;
		for (int dim = 0; dim < 2; dim++)
		{
			indexPoint2d[dim] = (contourModel2D[i][dim]);
		}
		if (i == 0)
		{
			figure->InvokeEvent(mitk::StartPlacementPlanarFigureEvent());
			figure->PlaceFigure(indexPoint2d);
		}
		else
		{
			figure->SetControlPoint(i, indexPoint2d, true);
		}
	}
	figure->SetProperty("initiallyplaced", mitk::BoolProperty::New(true));
	figure->InvokeEvent(mitk::EndPlacementPlanarFigureEvent());
	figure->SetPlaneGeometry(plane);

	//get the landmark points
	mitk::Point3D mraLandmark3D;
	mitk::Point2D mraLandmark;
	mitk::Point3D pcmriLandmark3D;
	mitk::Point2D pcmriLandmark;

	mitk::DataNode::Pointer pcmriPointNode = crimson::PCMRIUtils::getPcmriPointNode(pcmriNodeMagnitude);
	mitk::PointSet::Pointer pcmriPoint = dynamic_cast<mitk::PointSet*>(pcmriPointNode->GetData());
	auto test = pcmriPoint->GetPoint(0);
	pcmriLandmark3D[0] = pcmriPoint->GetPoint(0)[0];
	pcmriLandmark3D[1] = pcmriPoint->GetPoint(0)[1];
	pcmriLandmark3D[2] = pcmriPoint->GetPoint(0)[2];
	auto planePCMRI = contoursPCMRI[0]->GetPlaneGeometry();
	planePCMRI->Map(pcmriLandmark3D, pcmriLandmark);

	mitk::DataNode::Pointer mraNode = crimson::PCMRIUtils::getMraPointNode(node);
	mitk::PointSet::Pointer mraPoint = dynamic_cast<mitk::PointSet*>(mraNode->GetData());
	mraLandmark3D[0] = mraPoint->GetPoint(0)[0];
	mraLandmark3D[1] = mraPoint->GetPoint(0)[1];
	mraLandmark3D[2] = mraPoint->GetPoint(0)[2];

	plane->Map(mraLandmark3D, mraLandmark);


	mitk::Image::Pointer filteredImage = dynamic_cast<mitk::Image*>(pcmriNodePhase->GetData())->Clone();
	mitk::Image::Pointer filteredImageMagn = dynamic_cast<mitk::Image*>(pcmriNodeMagnitude->GetData())->Clone();

	int cardiacFrequency = 0;
	pcmriNodePhase->GetIntProperty("mapping.cardiacfrequency", cardiacFrequency);
	filteredImage->GetPropertyList()->SetIntProperty("mapping.cardiacfrequency", cardiacFrequency);

	bool flipped = false;
	pcmriNodeMagnitude->GetBoolProperty("mapping.imageFlipped", flipped);
	filteredImage->GetPropertyList()->SetBoolProperty("mapping.imageFlipped", flipped);

	filteredImage->GetPropertyList()->SetStringProperty("mapping.meshNodeUID", meshUID.c_str());


	//Calculate true values of velocities
	FloatImageType::Pointer pcmriImageScaled = FloatImageType::New();
	auto type = filteredImage->GetPixelType();
	if (type.GetComponentTypeAsString() == "short")
	{
		pcmriImageScaled = calculateVelocities<short>(filteredImage, filteredImageMagn, pcmriNodePhase->GetPropertyList());
	}
	else if (type.GetComponentTypeAsString() == "unsigned_short")
	{
		pcmriImageScaled = calculateVelocities<unsigned short>(filteredImage, filteredImageMagn, pcmriNodePhase->GetPropertyList());
	}
	else if (type.GetComponentTypeAsString() == "double")
	{
		pcmriImageScaled = calculateVelocities<double>(filteredImage, filteredImageMagn, pcmriNodePhase->GetPropertyList());
	}
	else
	{
		MITK_ERROR << "Unsupported image pixel type: short, unsigned short or double expected!";
		return nullptr;
	}
	FloatImageType::Pointer pcmriImageSmoothed = FloatImageType::New();

	//Gaussian smoothing
	double profileSmoothness = 0;
	int numberSlices = contoursPCMRI.size();
	node->GetDoubleProperty("mapping.smoothness", profileSmoothness);

	int numPhases = 0;
	int startIndex = 0;

	if (filteredImage->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", numPhases))
		startIndex = 2 * numPhases;

	if (profileSmoothness > 0)
	{
		pcmriImageSmoothed = gaussianSmooth(pcmriImageScaled.GetPointer(), pcmriNodePhase, profileSmoothness, contourNodes);
	}
	else
		pcmriImageSmoothed = pcmriImageScaled.GetPointer();


    // Create the mapping task
	auto mapTask = crimson::IPCMRIKernel::createMapTask(contoursPCMRI, figure.GetPointer(), mesh, face, pcmriLandmark,
		mraLandmark, pcmriImageSmoothed.GetPointer(), flipped, meshUID.c_str(), cardiacFrequency, startIndex);

    // Setup the node properties for the mapped model
	std::map<std::string, mitk::BaseProperty::Pointer> props;
	mitk::DataNode::Pointer prevMap = hm->getFirstDescendant(node, crimson::SolverSetupNodeTypes::PCMRIData());

	props["mapping.map_result"] = mitk::BoolProperty::New(true).GetPointer();

	if (prevMap) {
		props["name"] = prevMap->GetProperty("name");
		props["color"] = prevMap->GetProperty("color");
		props["opacity"] = prevMap->GetProperty("opacity");
	}
	else {
		props["name"] = mitk::StringProperty::New(node->GetName() + " Mapped").GetPointer();
	}
	

    // Launch the mapping task
	auto dataNodeTask = std::make_shared<crimson::CreateDataNodeAsyncTask>(
		mapTask, node, crimson::SolverSetupNodeTypes::BoundaryCondition(),
		crimson::SolverSetupNodeTypes::PCMRIData(), props);
	dataNodeTask->setDescription(std::string("Map ") + node->GetName());
	//dataNodeTask->setSilentFail(_preview);

	crimson::AsyncTaskManager::getInstance()->addTask(dataNodeTask, crimson::PCMRIUtils::getMappingTaskUID(node));

	return dataNodeTask;

}

void MapAction::Run(const QList<mitk::DataNode::Pointer>& selectedNodes)
{
	for (const mitk::DataNode::Pointer& node : selectedNodes) {
		Run(node);
	}
}

