
#include <memory>
#include <stdexcept>

#include "DicomReader.h"

#include "core/model/Image.h"
#include "core/model/Patient.h"
#include "core/model/Series.h"
#include "core/model/Study.h"
#include "dcmtk/dcmdata/dcfilefo.h"

DicomReader::DicomReader() {}

void DicomReader::readFile(const std::string& t_filePath)
{
  m_filePath = t_filePath;
  m_file = std::make_unique<DcmFileFormat>();
  if (m_file->loadFile(t_filePath.c_str()).bad()) {
    m_file.release();
    m_dataSet = nullptr;
    throw std::runtime_error("Cannot open file !");
  }
  m_dataSet = m_file->getDataset();
}

bool DicomReader::dataSetExist() const
{
  return m_dataSet != nullptr;
}

std::unique_ptr<Patient> DicomReader::getReadPatient() const
{
  auto t_patient = std::make_unique<Patient>();
  return t_patient;
}

std::unique_ptr<Study> DicomReader::getReadStudy() const
{
  auto t_study = std::make_unique<Study>();
  return t_study;
}

std::unique_ptr<Series> DicomReader::getReadSeries() const
{
  auto t_series = std::make_unique<Series>();
  return t_series;
}

std::unique_ptr<Image> DicomReader::getReadImage() const
{
  auto t_image = std::make_unique<Image>();
  return t_image;
}

std::string DicomReader::getTagFromDataSet(const DcmTagKey& tagKey) const
{
  auto t_tag = "";
  return t_tag;
}

bool DicomReader::isModalitySupported(const std::string& t_modality)
{
  return true;
}
