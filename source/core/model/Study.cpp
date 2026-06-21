
#include <algorithm>
#include <iterator>
#include <memory>

#include "Study.h"

#include "core/model/Series.h"

Study::Study() {}

Series* Study::addSeries(std::unique_ptr<Series> t_series, bool& t_new_series)
{
  auto index = findSeriesIndex(t_series.get());
  if (index == m_series.size()) {
    m_series.emplace_back(std::move(t_series));
    index = m_series.size() - 1;
  }
  m_series.at(index)->setIndex(index);
  return m_series.at(index).get();
}

std::size_t Study::findSeriesIndex(Series* t_series)
{
  auto it = std::find_if(m_series.begin(),
                         m_series.end(),
                         [&t_series](std::unique_ptr<Series>& series)
                         { return t_series->getUID() == series->getUID(); });

  return std::distance(m_series.begin(), it);
}
