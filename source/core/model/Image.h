
#pragma once

#include <cstddef>
#include <string>
class Series;

class Image
{
public:
  explicit Image();
  ~Image() = default;

private:
  std::size_t m_index = -1;
  Series* m_parent = {};
  std::string m_path = {};
  std::string m_sopInstanceUid = {};
  std::string m_classUid = {};
  std::string m_frameOfReferenceId = {};
  std::string m_modality = {};
};
