 /**
 * @file	Project\code\filter\vtkAluminiumMarkerPolyDataExtraction.cxx
 * @language	C++
 * @author	wuzhuobin jiejin2022@163.com
 * @date	2017/12/28
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *			This program is distributed in the hope that it will be useful, but	 *
 *			WITHOUT ANY WARRANTY; without even the implied warranty of			 * 
 *			MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.				 * 
 *			See the LICENSE for more detail.									 * 
 *			Copyright (c) WUZHUOBIN. All rights reserved.						 * 
 *			See COPYRIGHT for more detail.										 * 
 *			This software is distributed WITHOUT ANY WARRANTY; without even		 * 
 *			the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR	 * 
 *			PURPOSE.  See the above copyright notice for more information.		 *
 *			Internal usage only, without the permission of the author, please DO *
 *			NOT publish and distribute without the author's permission.  	     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
// me
#include "vtkAMPDEFilter2.h"


// vtk
#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMarchingCubes.h>



// itk
#include <itkResampleImageFilter.h>
#include <itkIdentityTransform.h>
#include <itkScalarImageKmeansImageFilter.h>
//#include <itkBinaryDilateImageFilter.h>
//#include <itkFlatStructuringElement.h>
//#include <itkOtsuThresholdImageFilter.h>
//#include <itkOtsuMultipleThresholdsImageFilter.h>
#include <itkBinaryMorphologicalOpeningImageFilter.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkThresholdImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkVTKImageToImageFilter.h>
//#include <itkMedianImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkLabelStatisticsImageFilter.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkLabelMapMaskImageFilter.h>
#include <itkMaskImageFilter.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkOtsuThresholdImageFilter.h>

vtkStandardNewMacro(vtkAMPDEFilter2);
void vtkAMPDEFilter2::SetKMeans(const std::vector<double>& k)
{
	this->KMeans = k;
	this->Modified();
}

vtkAMPDEFilter2::vtkAMPDEFilter2()
{
	this->SetNumberOfOutputPorts(2);
	this->VolumeTolerance = VTK_DOUBLE_MIN;
	this->TargetVolume = 0;
	this->ResampleSize[0] = 100;
	this->ResampleSize[1] = 100;
	this->ResampleSize[2] = 100;
	this->KMeans.push_back(-1000.0);
	this->KMeans.push_back(600);
	this->KMeans.push_back(2000);
	this->KMeans.push_back(4000);
	this->AluminiumMean = 2;
	this->MarchingCubes = vtkMarchingCubes::New();
}

int vtkAMPDEFilter2::FillOutputPortInformation(int port, vtkInformation * info)
{
	if (port != 1) {
		return Superclass::FillOutputPortInformation(port, info);
	}
	else {
		info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
		return 1;
	}
}

vtkAMPDEFilter2::~vtkAMPDEFilter2()
{
	this->KMeans.clear();
	this->MarchingCubes->Delete();
}

int vtkAMPDEFilter2::RequestData(vtkInformation * request, vtkInformationVector ** inputVector, vtkInformationVector * outputVector)
{
	// get the info objects
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
	vtkInformation *outInfo1 = outputVector->GetInformationObject(1);
	// get the input and output
	vtkImageData *input = vtkImageData::SafeDownCast(
		inInfo->Get(vtkDataObject::DATA_OBJECT()));
	vtkPolyData *output0 = vtkPolyData::SafeDownCast(
		outInfo0->Get(vtkDataObject::DATA_OBJECT()));
	vtkImageData *output1 = vtkImageData::SafeDownCast(
		outInfo1->Get(vtkDataObject::DATA_OBJECT()));
	int failed = 1;
	switch (input->GetScalarType())
	{
		vtkTemplateMacro(failed = this->ITK_Calculation<VTK_TT>(input, output1));
		//failed = this->ITK_Calculation(input, input);
	}
	if (failed) {
		vtkErrorMacro(<< "ITK_Calculation failed. ");
		return 1;
	}
	this->MarchingCubes->SetInputData(output1);
	this->MarchingCubes->GenerateValues(1, 1, 1);
	this->MarchingCubes->Update();
	output0->ShallowCopy(this->MarchingCubes->GetOutput());
	return 1;
}
//#include <itkImageFileWriter.h>

const unsigned short DIMENSION = 3;
typedef itk::Image<short, DIMENSION> LabelType;
// don't use char
//typedef itk::Image<signed char, 3> LabelType;
static bool ITK_FindSuitableLabel(LabelType *input, LabelType *output, double targetVolume, double volumeTolerance) {

	typedef itk::ConnectedComponentImageFilter < LabelType, LabelType >
		ConnectedComponentImageFilterType;
	typename ConnectedComponentImageFilterType::Pointer connectedComponentImageFilter =
		ConnectedComponentImageFilterType::New();
	connectedComponentImageFilter->SetInput(input);
	connectedComponentImageFilter->Update();
	//int count = connectedComponentImageFilter->GetObjectCount();

	//cout << "connected\n";
	//cout << "connected object " << count << '\n';

	typedef itk::LabelStatisticsImageFilter< LabelType, LabelType > LabelStatisticsImageFilterType;
	typename LabelStatisticsImageFilterType::Pointer labelStatisticsImageFilter = LabelStatisticsImageFilterType::New();
	labelStatisticsImageFilter->SetLabelInput(connectedComponentImageFilter->GetOutput());
	labelStatisticsImageFilter->SetInput(input);
	labelStatisticsImageFilter->Update();
	//cout << "number of labels: " << labelStatisticsImageFilter->GetNumberOfLabels() << '\n';
	//cout << "number of valid labes: " << labelStatisticsImageFilter->GetValidLabelValues().size() << '\n';

	typedef itk::LabelImageToLabelMapFilter <LabelType>
		LabelImageToLabelMapFilterType;
	typename LabelImageToLabelMapFilterType::Pointer labelImageToLabelMapFilter
		= LabelImageToLabelMapFilterType::New();
	labelImageToLabelMapFilter->SetInput(connectedComponentImageFilter->GetOutput());
	labelImageToLabelMapFilter->Update();

	//cout << "label\n";
	
	typedef LabelStatisticsImageFilterType::ValidLabelValuesContainerType ValidLabelValuesType;
	typedef LabelStatisticsImageFilterType::LabelPixelType                LabelPixelType;

	double volumeSize = 1;
	double relativeTolerance = itk::NumericTraits<double>::max();

	for (int i = 0; i < DIMENSION; ++i) {
		volumeSize *= input->GetSpacing()[i];
	}

	for (typename ValidLabelValuesType::const_iterator vIt = labelStatisticsImageFilter->GetValidLabelValues().begin();
		vIt != labelStatisticsImageFilter->GetValidLabelValues().end();
		++vIt)
	{
		if (labelStatisticsImageFilter->HasLabel(*vIt))
		{
			LabelPixelType labelValue = *vIt;
			double volume = volumeSize * labelStatisticsImageFilter->GetCount(labelValue);
			//cout << "Volume is " << volume << '\n';
			double tolerance = itk::Math::abs(volume - targetVolume);
			if (tolerance < volumeTolerance && 
				tolerance < relativeTolerance) {
				//cout << "Find volume is " << volume << '\n';
				relativeTolerance = tolerance;
				typedef LabelImageToLabelMapFilterType::OutputImageType LabelMapType;
				typedef itk::LabelMapMaskImageFilter<LabelMapType, LabelType > LabelMapMaskImageFilter;
				typename LabelMapMaskImageFilter::Pointer labelMapMaskImageFilter = LabelMapMaskImageFilter::New();
				labelMapMaskImageFilter->SetInput(labelImageToLabelMapFilter->GetOutput());
				labelMapMaskImageFilter->SetFeatureImage(input);
				labelMapMaskImageFilter->SetLabel(*vIt);
				labelMapMaskImageFilter->SetBackgroundValue(0);
				labelMapMaskImageFilter->Update();
				output->Graft(labelMapMaskImageFilter->GetOutput());
				return true;
			}
		}
	}
	return false;
}

template<typename ScalarType>
int vtkAMPDEFilter2::ITK_Calculation(vtkImageData* input, vtkImageData* output)
{
	//typedef short ScalarType;
	typedef itk::Image<ScalarType, DIMENSION> ImageType;

	typedef itk::VTKImageToImageFilter<ImageType> VTKImageToImageFilter;
	typename VTKImageToImageFilter::Pointer vtkImageToImageFilter = 
		VTKImageToImageFilter::New();
	vtkImageToImageFilter->SetInput(input);
	vtkImageToImageFilter->Update();

	// downsmaple to speed up. 
	typedef itk::ResampleImageFilter<ImageType, ImageType> ResampleImageFilter;
	typedef itk::IdentityTransform<double, DIMENSION> ResampleTransform;
	typename ResampleImageFilter::Pointer resampleImageFilter =
		ResampleImageFilter::New();
	resampleImageFilter->SetInput(vtkImageToImageFilter->GetOutput());
	const typename ImageType::PointType& inputOrigin =
		resampleImageFilter->GetInput()->GetOrigin();
	const typename ImageType::SpacingType& inputSpacing =
		resampleImageFilter->GetInput()->GetSpacing();
	const typename ImageType::SizeType& inputSize =
		resampleImageFilter->GetInput()->GetLargestPossibleRegion().GetSize();
	typename ImageType::SizeType outputSize;
	// downsample size.
	//outputSize.Fill(100);
	outputSize[0] = this->ResampleSize[0];
	outputSize[1] = this->ResampleSize[1];
	outputSize[2] = this->ResampleSize[2];
	typename ImageType::SpacingType outputSpacing;
	for (int i = 0; i < DIMENSION; ++i)
	{
		outputSpacing[i] = 1.0 * inputSize[i] * inputSpacing[i] / outputSize[i];
	}
	resampleImageFilter->SetOutputOrigin(inputOrigin);
	resampleImageFilter->SetOutputSpacing(outputSpacing);
	resampleImageFilter->SetSize(outputSize);
	resampleImageFilter->SetTransform(ResampleTransform::New());
	resampleImageFilter->Update();

	//typedef itk::ImageFileWriter<LabelType> LabelFileWriter;
	//typename LabelFileWriter::Pointer labelWriter = LabelFileWriter::New();
	//itk::ImageFileWriter<ImageType>::Pointer writer = itk::ImageFileWriter<ImageType>::New();
	//writer->SetFileName("resameple.nii.gz");
	//writer->SetInput(resampleImageFilter->GetOutput());
	//writer->Write();

	//cout << "resample\n";
	typedef itk::ScalarImageKmeansImageFilter<ImageType, LabelType> ScalarImageKmeansImageFilter;
	typename ScalarImageKmeansImageFilter::Pointer scalarImageKmeansImageFilter =
		ScalarImageKmeansImageFilter::New();
	scalarImageKmeansImageFilter->SetInput(resampleImageFilter->GetOutput());
	for (double mean : this->KMeans) {
		scalarImageKmeansImageFilter->AddClassWithInitialMean(mean);
	}
	scalarImageKmeansImageFilter->UseNonContiguousLabelsOff();
	scalarImageKmeansImageFilter->Update();
	//labelWriter->SetInput(scalarImageKmeansImageFilter->GetOutput());
	//labelWriter->SetFileName("kmeans.nii.gz");
	//labelWriter->Write();
	//cout << "kmeans\n";
	//typedef itk::OtsuMultipleThresholdsImageFilter<ImageType, ImageType> OtsuMultipleThresholdFilter;
	//typename OtsuMultipleThresholdFilter::Pointer otsuMultipleThresholdFilter =
	//	OtsuMultipleThresholdFilter::New();
	//otsuMultipleThresholdFilter->SetInput(resampleImageFilter->GetOutput());
	//otsuMultipleThresholdFilter->SetNumberOfThresholds(2);
	//otsuMultipleThresholdFilter->Update();

	//typedef itk::ThresholdImageFilter<LabelType> ThresholdImageFilter;
	typedef itk::BinaryThresholdImageFilter<LabelType, LabelType> ThresholdImageFilter;
	typename ThresholdImageFilter::Pointer thresholdImageFilter =
		ThresholdImageFilter::New();
	thresholdImageFilter->SetInput(scalarImageKmeansImageFilter->GetOutput());
	thresholdImageFilter->SetLowerThreshold(this->AluminiumMean);
	thresholdImageFilter->SetUpperThreshold(this->AluminiumMean);
	thresholdImageFilter->SetOutsideValue(0);
	thresholdImageFilter->SetInsideValue(1);
	thresholdImageFilter->Update();
	
	//typedef itk::ConnectedComponentImageFilter < LabelType, LabelType >
	//	ConnectedComponentImageFilterType;
	//typename ConnectedComponentImageFilterType::Pointer connectedComponentImageFilter =
	//	ConnectedComponentImageFilterType::New();
	//connectedComponentImageFilter->SetInput(thresholdImageFilter->GetOutput());
	//connectedComponentImageFilter->Update();
	////int count = connectedComponentImageFilter->GetObjectCount();

	////cout << "connected\n";
	////cout << "connected object " << count << '\n';

	//typedef itk::LabelStatisticsImageFilter< LabelType, LabelType > LabelStatisticsImageFilterType;
	//typename LabelStatisticsImageFilterType::Pointer labelStatisticsImageFilter = LabelStatisticsImageFilterType::New();
	//labelStatisticsImageFilter->SetLabelInput(connectedComponentImageFilter->GetOutput());
	//labelStatisticsImageFilter->SetInput(thresholdImageFilter->GetOutput());
	//labelStatisticsImageFilter->Update();
	////cout << "number of labels: " << labelStatisticsImageFilter->GetNumberOfLabels() << '\n';
	////cout << "number of valid labes: " << labelStatisticsImageFilter->GetValidLabelValues().size() << '\n';

	//typedef itk::LabelImageToLabelMapFilter <LabelType>
	//	LabelImageToLabelMapFilterType;
	//typename LabelImageToLabelMapFilterType::Pointer labelImageToLabelMapFilter
	//	= LabelImageToLabelMapFilterType::New();
	//labelImageToLabelMapFilter->SetInput(connectedComponentImageFilter->GetOutput());
	//labelImageToLabelMapFilter->Update();

	////cout << "label\n";
	//
	//typedef LabelStatisticsImageFilterType::ValidLabelValuesContainerType ValidLabelValuesType;
	//typedef LabelStatisticsImageFilterType::LabelPixelType                LabelPixelType;

	//typename LabelType::Pointer aluminiumMarker = nullptr;
	//double volumeSize = 1;
	//double relativeTolerance = itk::NumericTraits<double>::max();

	//for (int i = 0; i < DIMENSION; ++i) {
	//	volumeSize *= resampleImageFilter->GetOutput()->GetSpacing()[i];
	//}

	//for (typename ValidLabelValuesType::const_iterator vIt = labelStatisticsImageFilter->GetValidLabelValues().begin();
	//	vIt != labelStatisticsImageFilter->GetValidLabelValues().end();
	//	++vIt)
	//{
	//	if (labelStatisticsImageFilter->HasLabel(*vIt))
	//	{
	//		LabelPixelType labelValue = *vIt;
	//		double volume = volumeSize * labelStatisticsImageFilter->GetCount(labelValue);
	//		//cout << "Volume is " << volume << '\n';
	//		double tolerance = itk::Math::abs(volume - this->TargetVolume);
	//		if (tolerance < this->VolumeTolerance && 
	//			tolerance < relativeTolerance) {
	//			//cout << "Find volume is " << volume << '\n';
	//			relativeTolerance = tolerance;
	//			typedef LabelImageToLabelMapFilterType::OutputImageType LabelMapType;
	//			typedef itk::LabelMapMaskImageFilter<LabelMapType, LabelType > LabelMapMaskImageFilter;
	//			typename LabelMapMaskImageFilter::Pointer labelMapMaskImageFilter = LabelMapMaskImageFilter::New();
	//			labelMapMaskImageFilter->SetInput(labelImageToLabelMapFilter->GetOutput());
	//			labelMapMaskImageFilter->SetFeatureImage(thresholdImageFilter->GetOutput());
	//			labelMapMaskImageFilter->SetLabel(*vIt);
	//			labelMapMaskImageFilter->SetBackgroundValue(0);
	//			labelMapMaskImageFilter->Update();
	//			aluminiumMarker = LabelType::New();
	//			aluminiumMarker->Graft(labelMapMaskImageFilter->GetOutput());
	//		}
	//	}
	//}

	//if (aluminiumMarker.IsNull()) {
	//	vtkErrorMacro(<< "Cannot find the aluminium marker. ");
	//	return 1;
	//}
	typename LabelType::Pointer aluminiumMarker = LabelType::New();
	if (!ITK_FindSuitableLabel(thresholdImageFilter->GetOutput(), aluminiumMarker, this->TargetVolume, this->VolumeTolerance)) {
		vtkErrorMacro(<< "Cannot find the aluminium marker. ");
		return 1;
	}

	typedef itk::FlatStructuringElement<DIMENSION> FlatStructuringElement;
	typename FlatStructuringElement::RadiusType elementRadius;
	elementRadius.Fill(1);
	FlatStructuringElement boxElement = FlatStructuringElement::Box(elementRadius);
	typedef typename itk::BinaryDilateImageFilter<LabelType, LabelType, FlatStructuringElement> BinaryDilateImageFilter;
	typename BinaryDilateImageFilter::Pointer binaryDilateImageFilter = BinaryDilateImageFilter::New();
	binaryDilateImageFilter->SetInput(aluminiumMarker);
	binaryDilateImageFilter->SetKernel(boxElement);
	binaryDilateImageFilter->SetDilateValue(1);
	binaryDilateImageFilter->Update();

	typedef itk::NearestNeighborInterpolateImageFunction<LabelType> Interpolator;
	typename Interpolator::Pointer interpoltor = Interpolator::New();
	typedef itk::ResampleImageFilter<LabelType, LabelType> ResampleLabelFilter;
	typename ResampleLabelFilter::Pointer resampleLabelFilter = ResampleLabelFilter::New();
	resampleLabelFilter->SetInput(binaryDilateImageFilter->GetOutput());
	resampleLabelFilter->SetOutputOrigin(inputOrigin);
	resampleLabelFilter->SetOutputSpacing(inputSpacing);
	resampleLabelFilter->SetSize(inputSize);
	resampleLabelFilter->SetTransform(ResampleTransform::New());
	resampleLabelFilter->SetInterpolator(interpoltor);
	resampleLabelFilter->Update();

	

	//labelWriter->SetInput(aluminiumMarker);
	//labelWriter->SetFileName("aluminium_marker.nii.gz");
	//labelWriter->Write();
	//labelWriter->SetInput(binaryDilateImageFilter->GetOutput());
	//labelWriter->SetFileName("Dilated.nii.gz");
	//labelWriter->Write();
	//labelWriter->SetInput(resampleLabelFilter->GetOutput());
	//labelWriter->SetFileName("Dilated_resample.nii.gz");
	//labelWriter->Write();

	typedef itk::MaskImageFilter<ImageType, LabelType> MaskImageFilter;
	typename MaskImageFilter::Pointer maskImageFilter =
		MaskImageFilter::New();
	maskImageFilter->SetInput(vtkImageToImageFilter->GetOutput());
	maskImageFilter->SetMaskImage(resampleLabelFilter->GetOutput());
	maskImageFilter->Update();
	//writer->SetInput(maskImageFilter->GetOutput());
	//writer->SetFileName("Mask.nii.gz");
	//writer->Write();

	typedef itk::OtsuThresholdImageFilter<ImageType, LabelType, LabelType> OtsuThresholdImageFilter;
	typename OtsuThresholdImageFilter::Pointer otsu = OtsuThresholdImageFilter::New();
	otsu->SetInput(maskImageFilter->GetOutput());
	otsu->SetMaskImage(resampleLabelFilter->GetOutput());
	otsu->SetOutsideValue(1);
	otsu->SetInsideValue(0);
	otsu->SetMaskValue(1);
	otsu->Update();

	typedef typename itk::BinaryBallStructuringElement<LabelType::PixelType, 3> BinaryBallStructuringElement;
	typename BinaryBallStructuringElement ball;
	ball.SetRadius(3);
	ball.CreateStructuringElement();
	

	typedef itk::BinaryMorphologicalOpeningImageFilter<LabelType, LabelType, BinaryBallStructuringElement> BinaryMorphologicalOpeningImageFilter;
	typename BinaryMorphologicalOpeningImageFilter::Pointer openFilter = BinaryMorphologicalOpeningImageFilter::New();
	openFilter->SetInput(otsu->GetOutput());
	openFilter->SetForegroundValue(1);
	openFilter->SetBackgroundValue(0);
	openFilter->SetKernel(ball);
	openFilter->Update();

	ITK_FindSuitableLabel(openFilter->GetOutput(), aluminiumMarker, this->TargetVolume, this->VolumeTolerance);
/*	labelWriter->SetInput(aluminiumMarker);
	labelWriter->SetFileName("aluminium_marker.nii.gz");
	labelWriter->Write()*/;


	typedef itk::ImageToVTKImageFilter<LabelType> ImageToVTKImageFilter;
	typename ImageToVTKImageFilter::Pointer imageToVTKImageFilter =
		ImageToVTKImageFilter::New();
	imageToVTKImageFilter->SetInput(aluminiumMarker);
	imageToVTKImageFilter->Update();
	output->DeepCopy(imageToVTKImageFilter->GetOutput());
	return 0;
}

int vtkAMPDEFilter2::FillInputPortInformation(int port, vtkInformation * info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
	return 1;
}

void vtkAMPDEFilter2::PrintSelf(ostream & os, vtkIndent indent)
{
	Superclass::PrintSelf(os, indent);
	os << indent << "VolumeTolerance: " << this->VolumeTolerance << '\n';
	os << indent << "TargetVolume: " << this->TargetVolume << '\n';
	os << indent << "ResampleSize: " <<
		this->ResampleSize[0] << ' ' <<
		this->ResampleSize[1] << ' ' <<
		this->ResampleSize[2] << ' ' << '\n';
	os << indent << "KMeans: ";
	for (double v : this->KMeans) {
		os << v << ' ';
	}
	os << '\n';
	os << indent << "AluminiumMean: " << this->AluminiumMean;
}

vtkImageData * vtkAMPDEFilter2::GetOutputImage()
{
	return vtkImageData::SafeDownCast(this->GetOutputDataObject(1));
}

vtkAlgorithmOutput * vtkAMPDEFilter2::GetOutputPortImage()
{
	return this->GetOutputPort(1);
}
