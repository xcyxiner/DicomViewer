
#pragma once
#include "infrastructure/dicom_io/IDicomReader.h"
#include <vtkDICOMReader.h>
#include <vtkSmartPointer.h>
class VTKDicomAdaptReader:public IDicomReader
{
public:
    explicit VTKDicomAdaptReader();
    ~VTKDicomAdaptReader()=default;
public:
    void open(const std::string& filePath) override;
    Series readSeries(const std::string& path) override;
    std::unique_ptr<Frame> readFrameInfo( int index) override;
    IFrameCache::FramePtr readFrame(int index) override;
    void close() override;
private:
    std::string m_filePath;
    vtkSmartPointer<vtkDICOMReader> m_reader;
};
