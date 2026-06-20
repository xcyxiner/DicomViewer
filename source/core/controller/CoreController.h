
#pragma once

#include <memory>
#include <string>

#include "core/lib/DicomReader.h"
#include "core/model/CoreRepository.h"

class CoreController
{
public:
  explicit CoreController();
  ~CoreController() = default;

public:
  void readData(const std::string& t_filePath) const;

private:
  std::unique_ptr<DicomReader> m_dicomReader;
  std::unique_ptr<CoreRepository> m_coreRepository;

  void insertDataInRepo() const;
};
