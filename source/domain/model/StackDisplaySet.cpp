
#include "StackDisplaySet.h"

StackDisplaySet::StackDisplaySet() {}

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
