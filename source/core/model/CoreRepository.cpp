
#include <algorithm>
#include <utility>

#include "CoreRepository.h"

#include "core/model/Patient.h"

CoreRepository::CoreRepository() {}

void CoreRepository::addPatient(std::unique_ptr<Patient> t_patient)
{
  auto index = findPatient(t_patient.get());
  if (index == m_patients.size()) {
    m_patients.emplace_back(std::move(t_patient));
    index = m_patients.size() - 1;
  }
  m_lastPatient = m_patients.at(index).get();
  m_lastPatient->SetIndex(index);
}

void CoreRepository::addStudy(std::unique_ptr<Study> t_study)
{
  t_study->setParentObj(m_lastPatient);
  m_lastStudy = m_lastPatient->addStudy(std::move(t_study));
}

void CoreRepository::addSeries(std::unique_ptr<Series> t_series)
{
  if (!t_series) {
    return;
  }
  t_series->setParentObj(m_lastStudy);
  m_lastSeries = m_lastStudy->addSeries(std::move(t_series), m_new_Series);
}

void CoreRepository::addImg(std::unique_ptr<Image> t_image) {}

void CoreRepository::deletePatient(Patient* t_patient)
{
  const auto index = findPatient(t_patient);
  if (index != m_patients.size()) {
    m_patients.erase(m_patients.begin() + index);
  }
}

void CoreRepository::resetLastPatient()
{
  m_lastPatient = nullptr;
  m_lastStudy = nullptr;
  m_lastSeries = nullptr;
  m_lastImg = nullptr;
  m_new_Series = false;
}

std::size_t CoreRepository::findPatient(Patient* t_patient)
{
  const auto it =
      std::find_if(m_patients.begin(),
                   m_patients.end(),
                   [&t_patient](const std::unique_ptr<Patient>& patient)
                   { return t_patient->getId() == patient->getId(); });
  return std::distance(m_patients.begin(), it);
}
