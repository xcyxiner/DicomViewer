
#pragma once

#include <cstddef>
#include <string>
class Study;

class Series
{
public:
  explicit Series();
  ~Series() = default;

public:
  int getIndex() const { return m_index; }

  std::string getUID() const { return m_uid; }

  std::string getDesctiption() const { return m_desctiption; }

  std::string getDate() const { return m_date; }

  std::string getNumber() const { return m_number; }

  Study* getParentObj() const { return m_parent; }

  void setIndex(const int& t_index) { m_index = t_index; }

  void setUID(const std::string& t_uid) { m_uid = t_uid; }

  void setDesctiption(const std::string& t_desctiption)
  {
    m_desctiption = t_desctiption;
  }

  void setDate(const std::string& t_date) { m_date = t_date; }

  void setNumber(const std::string& t_number) { m_number = t_number; }

  void setParentObj(Study* t_parent) { m_parent = t_parent; }

private:
  std::size_t m_index = -1;
  Study* m_parent = {};
  std::string m_uid = {};
  std::string m_desctiption = {};
  std::string m_date = {};
  std::string m_number = {};
};
