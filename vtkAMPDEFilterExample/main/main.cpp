// @file main.cpp
// c
#include <stdio.h>
// me
#include "vtkAMPDEFilter.h"
// vtk
#include <vtkNew.h>
#include <vtkNIFTIImageReader.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkPolyDataWriter.h>


int main(int argc, char **argv)
{
	//argc = 3;
	//argv[1] = "C:/Users/jieji/Desktop/DentalData/fouth_marker1.nii.gz";
	//argv[2] = "output.vtk";
	if (argc != 3) {
		std::cerr << "2 files path were needed. \n";
		return 1;
	}
	vtkNew<vtkNIFTIImageReader> reader;
	reader->SetFileName(argv[1]);
	reader->Update();

	vtkNew<vtkAMPDEFilter> filer;
	filer->SetInputConnection(reader->GetOutputPort());
	filer->SetKMeans(std::vector<double>{-1000, 600, 2000});
	filer->SetAluminiumMean(2);
	filer->SetTargetVolume(3134);
	filer->SetVolumeTolerance(1000);
	filer->Update();

	vtkNew<vtkPolyDataWriter> writer;
	writer->SetInputConnection(filer->GetOutputPort());
	writer->SetFileName(argv[2]);
	writer->Write();

	vtkNew<vtkNIFTIImageWriter> niftiWriter;
	niftiWriter->SetInputConnection(filer->GetOutputPortImage());
	niftiWriter->SetFileName((std::string(argv[2]) + ".nii").c_str());
	niftiWriter->Write();

	std::cout << "Press any keys to continue...\n";
	std::cin.get();
	return 0;
}