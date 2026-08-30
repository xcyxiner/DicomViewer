
#include "VTKDicomAdaptReader.h"

#include <vtkDICOMMetaData.h>
#include <vtkDICOMTagPath.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkStringArray.h>

VTKDicomAdaptReader::VTKDicomAdaptReader()
{
  m_reader = vtkSmartPointer<vtkDICOMReader>::New();
}

void VTKDicomAdaptReader::open(const std::string& filePath)
{
  m_filePath = filePath;
  m_reader->SetFileName(m_filePath.c_str());
  m_reader->Update();
}

Series VTKDicomAdaptReader::readSeries(const std::string& path)
{
  return Series();
}

std::unique_ptr<Frame> VTKDicomAdaptReader::readFrameInfo(int index)
{
  return std::unique_ptr<Frame>();
}

IFrameCache::FramePtr VTKDicomAdaptReader::readFrame(int index)
{
  if (!m_reader) {
    return nullptr;
  }
  if (!m_reader->GetOutput()) {
    return nullptr;
  }
  auto imageData = m_reader->GetOutput();

  vtkDICOMMetaData* meta = m_reader->GetMetaData();

  std::string sopUID =
      meta->GetAttributeValue(0, DC::SOPInstanceUID).AsString();
  vtkSmartPointer<vtkStringArray> uidArray =
      vtkSmartPointer<vtkStringArray>::New();
  uidArray->SetName("SOPInstanceUIDs");
  uidArray->InsertNextValue(sopUID);

  imageData->GetFieldData()->AddArray(uidArray);

  // 提取窗宽窗位 (0028,1050 / 0028,1051) — 多值时取第一个
  vtkDICOMValue wcValue = meta->GetAttributeValue(0, DC::WindowCenter);
  vtkDICOMValue wwValue = meta->GetAttributeValue(0, DC::WindowWidth);
  if (wcValue.IsValid() && wwValue.IsValid()) {
    auto wcArray = vtkSmartPointer<vtkDoubleArray>::New();
    wcArray->SetName("WindowCenter");
    wcArray->InsertNextValue(wcValue.AsDouble());
    imageData->GetFieldData()->AddArray(wcArray);

    auto wwArray = vtkSmartPointer<vtkDoubleArray>::New();
    wwArray->SetName("WindowWidth");
    wwArray->InsertNextValue(wwValue.AsDouble());
    imageData->GetFieldData()->AddArray(wwArray);
  }

  return imageData;
}

void VTKDicomAdaptReader::close() {}
