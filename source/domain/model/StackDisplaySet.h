
#pragma once
#include <vector>
#include <string>
#include "DisplaySettings.h"
class StackDisplaySet 
{
public:
    explicit StackDisplaySet();
    ~StackDisplaySet()=default;
public:

private:
    std::vector<std::string> frameUids;
    int currentIndex;
    DisplaySettings displaySettings;
    // 序列缓存键（SeriesInstanceUID）—— 用于从 IFrameCache 体槽位
    // 取整幅体（2D 抽层渲染与 MPR setVolume 共用同一份体数据）。
    std::string seriesKey;
public:

  // --- Getters & Setters (auto-generated) ---
  int getCurrentIndex() const;
  void setCurrentIndex(int currentIndex);
  const std::vector<std::string>& getFrameUids() const;
  void setFrameUids(const std::vector<std::string>& frameUids);

  const DisplaySettings& getDisplaySettings() const;
  void setDisplaySettings(const DisplaySettings& displaySettings);

  const std::string& getSeriesKey() const;
  void setSeriesKey(const std::string& seriesKey);

};
