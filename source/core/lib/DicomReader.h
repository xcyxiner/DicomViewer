
#pragma once
#include <memory>
#include <string>

#include <dcmtk/dcmdata/dcfilefo.h>

#include "core/model/Image.h"
#include "core/model/Patient.h"
#include "core/model/Series.h"
#include "core/model/Study.h"
#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dctagkey.h"

class DicomReader
{
public:
  explicit DicomReader();
  ~DicomReader() = default;

public:
  void readFile(const std::string& t_filePath);
  bool dataSetExist() const;
  std::unique_ptr<Patient> getReadPatient() const;
  std::unique_ptr<Study> getReadStudy() const;
  std::unique_ptr<Series> getReadSeries() const;
  std::unique_ptr<Image> getReadImage() const;

private:
  std::string m_filePath;
  DcmDataset* m_dataSet;
  std::unique_ptr<DcmFileFormat> m_file;

  std::string getTagFromDataSet(const DcmTagKey& tagKey) const;
  static bool isModalitySupported(const std::string& t_modality);
};
