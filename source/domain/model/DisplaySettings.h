
#pragma once

class DisplaySettings 
{
public:
    explicit DisplaySettings();
    ~DisplaySettings()=default;

private:
    double windowWidth=400;
    double windowCenter=40;
    
public:
  // --- Getters & Setters (auto-generated) ---
  double getWindowWidth() const;
  void setWindowWidth(double windowWidth);

  double getWindowCenter() const;
  void setWindowCenter(double windowCenter);
};
