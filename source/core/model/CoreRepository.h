
#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/model/Image.h"
#include "core/model/Patient.h"
#include "core/model/Series.h"
#include "core/model/Study.h"

class CoreRepository
{
public:
  explicit CoreRepository();
  ~CoreRepository() = default;

public:
  Patient* getLastPatient() const { return m_lastPatient; }

  Study* getLastStudy() const { return m_lastStudy; }

  Series* getLastSeries() const { return m_lastSeries; }

  Image* getLastImage() const { return m_lastImg; }

  void addPatient(std::unique_ptr<Patient> t_patient);
  void addStudy(std::unique_ptr<Study> t_study);
  void addSeries(std::unique_ptr<Series> t_series);
  void addImg(std::unique_ptr<Image> t_image);

  void deletePatient(Patient* t_patient);

  void deleteAllPatient() { m_patients.clear(); }

  void resetLastPatient();
  std::size_t findPatient(Patient* t_patient);

private:
  std::vector<std::unique_ptr<Patient>> m_patients = {};
  Patient* m_lastPatient = {};
  Study* m_lastStudy = {};
  Series* m_lastSeries = {};
  Image* m_lastImg = {};

  bool m_new_Series = false;
};
