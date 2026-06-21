
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

#include "DicomReader.h"

#include "core/model/Image.h"
#include "core/model/Patient.h"
#include "core/model/Series.h"
#include "core/model/Study.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dctagkey.h"
#include "dcmtk/ofstd/ofstring.h"

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
  const auto age = getTagFromDataSet(DCM_PatientAge);
  t_patient->setAge(age.empty() ? 0 : std::stoi(age));
  t_patient->setBirthDate(getTagFromDataSet(DCM_PatientBirthDate));
  t_patient->setId(getTagFromDataSet(DCM_PatientID));
  t_patient->setName(getTagFromDataSet(DCM_PatientName));
  return t_patient;
}

std::unique_ptr<Study> DicomReader::getReadStudy() const
{
  auto t_study = std::make_unique<Study>();
  t_study->setID(getTagFromDataSet(DCM_StudyID));
  t_study->setDate(getTagFromDataSet(DCM_StudyDate));
  t_study->setDesctiption(getTagFromDataSet(DCM_StudyDescription));
  t_study->setUID(getTagFromDataSet(DCM_StudyInstanceUID));
  return t_study;
}

std::unique_ptr<Series> DicomReader::getReadSeries() const
{
  auto t_series = std::make_unique<Series>();
  t_series->setUID(getTagFromDataSet(DCM_SeriesInstanceUID));
  t_series->setDate(getTagFromDataSet(DCM_SeriesDate));
  t_series->setDesctiption(getTagFromDataSet(DCM_SeriesDescription));
  auto series_number = getTagFromDataSet(DCM_SeriesNumber);
  t_series->setNumber(series_number.empty() ? "0" : series_number);
  return t_series;
}

std::unique_ptr<Image> DicomReader::getReadImage() const
{
  auto t_image = std::make_unique<Image>();
  return t_image;
}

std::string DicomReader::getTagFromDataSet(const DcmTagKey& tagKey) const
{
  try {
    OFString result;
    m_dataSet->findAndGetOFStringArray(tagKey, result);
    return result.c_str();
  } catch (std::exception& ex) {
    return {};
  }
}

bool DicomReader::isModalitySupported(const std::string& t_modality)
{
  return t_modality != "PR" && t_modality != "KO" && t_modality != "SR";
}
