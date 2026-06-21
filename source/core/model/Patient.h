
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "core/model/Study.h"

class Patient
{
public:
  explicit Patient();
  ~Patient() = default;

public:
  std::string getId() const { return m_id; }

  std::string getName() const { return m_name; }

  int getAge() const { return m_age; }

  std::string getBirthDate() const { return m_brithDate; }

  int getIndex() const { return m_index; }

  std::vector<std::unique_ptr<Study>>& getStudies() { return m_studies; }

  void setId(const std::string& t_id) { m_id = t_id; }

  void setName(const std::string& t_name) { m_name = t_name; }

  void setAge(const int& t_age) { m_age = t_age; }

  void setBirthDate(const std::string& t_birthDate)
  {
    m_brithDate = t_birthDate;
  }

  void SetIndex(const int& t_index) { m_index = t_index; }

public:
  Study* addStudy(std::unique_ptr<Study> t_study);
  std::size_t findStudyIndex(Study* t_study);

private:
  std::size_t m_index = -1;
  std::string m_id = {};
  std::string m_name = {};
  int m_age = {};
  std::string m_brithDate = {};
  std::vector<std::unique_ptr<Study>> m_studies = {};
};
