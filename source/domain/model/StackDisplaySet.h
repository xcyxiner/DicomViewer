
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
public:

  // --- Getters & Setters (auto-generated) ---
  int getCurrentIndex() const;
  void setCurrentIndex(int currentIndex);
  const std::vector<std::string>& getFrameUids() const;
  void setFrameUids(const std::vector<std::string>& frameUids);
   
  const DisplaySettings& getDisplaySettings() const;
  void setDisplaySettings(const DisplaySettings& displaySettings);

};
