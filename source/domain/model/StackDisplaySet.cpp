
#include "StackDisplaySet.h"

StackDisplaySet::StackDisplaySet()
    : currentIndex(0)
{
}

// --- StackDisplaySet Getters & Setters (auto-generated) ---
const DisplaySettings& StackDisplaySet::getDisplaySettings() const
{
  return displaySettings;
}

void StackDisplaySet::setDisplaySettings(const DisplaySettings& displaySettings)
{
  this->displaySettings = displaySettings;
}

// --- StackDisplaySet Getters & Setters (auto-generated) ---
const std::vector<std::string>& StackDisplaySet::getFrameUids() const
{
  return frameUids;
}

void StackDisplaySet::setFrameUids(const std::vector<std::string>& frameUids)
{
  this->frameUids = frameUids;
}

// --- StackDisplaySet Getters & Setters (auto-generated) ---
int StackDisplaySet::getCurrentIndex() const
{
  return currentIndex;
}

void StackDisplaySet::setCurrentIndex(int currentIndex)
{
  this->currentIndex = currentIndex;
}

// --- 序列缓存键 (U3: 体槽位取体入口的 key) ---
const std::string& StackDisplaySet::getSeriesKey() const
{
  return seriesKey;
}

void StackDisplaySet::setSeriesKey(const std::string& seriesKey)
{
  this->seriesKey = seriesKey;
}
