
#include <exception>
#include <memory>
#include <stdexcept>

#include "CoreController.h"

#include "core/lib/DicomReader.h"

CoreController::CoreController()
{
  m_dicomReader = std::make_unique<DicomReader>();
  m_coreRepository = std::make_unique<CoreRepository>();
}

void CoreController::readData(const std::string& t_filePath) const
{
  try {
    m_dicomReader->readFile(t_filePath);
    auto flag = m_dicomReader->dataSetExist();
    if (flag) {
      insertDataInRepo();
    } else {
      throw std::runtime_error("File not Support!");
    }
  } catch (std::exception& ex) {
  }
}

void CoreController::insertDataInRepo() const
{
  m_coreRepository->resetLastPatient();
  m_coreRepository->addPatient(m_dicomReader->getReadPatient());
  m_coreRepository->addStudy(m_dicomReader->getReadStudy());
  m_coreRepository->addSeries(m_dicomReader->getReadSeries());
  if (m_coreRepository->getLastSeries()) {}
}
