
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "core/model/Series.h"

class Patient;

class Study
{
public:
  explicit Study();
  ~Study() = default;

public:
  std::string getUID() const { return m_uid; }

  std::string getId() const { return m_id; }

  int getIndex() const { return m_index; }

  std::string getDesctiption() const { return m_desctiption; }

  std::string getDate() const { return m_date; }

  std::vector<std::unique_ptr<Series>>& getSeries() { return m_series; };

  Patient* getParentObj() const { return m_parent; }

  void setParentObj(Patient* t_parent) { m_parent = t_parent; }

  void setIndex(const int& t_index) { m_index = t_index; }

  void setUID(const std::string& t_uid) { m_uid = t_uid; }

  void setID(const std::string& t_id) { m_id = t_id; }

  void setDesctiption(const std::string& t_desctiption)
  {
    m_desctiption = t_desctiption;
  }

  void setDate(const std::string& t_date) { m_date = t_date; }

public:
  Series* addSeries(std::unique_ptr<Series> t_series, bool& t_new_series);

  std::size_t findSeriesIndex(Series* t_series);

private:
  std::size_t m_index = -1;
  Patient* m_parent = {};
  std::string m_uid = {};
  std::string m_id = {};
  std::string m_desctiption = {};
  std::string m_date = {};

  std::vector<std::unique_ptr<Series>> m_series = {};
};
