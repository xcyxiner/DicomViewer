
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class Frame
{
public:
  explicit Frame();
  ~Frame() = default;


 
private:
   std::vector<uint16_t> pixels;
   int rows;
   int cols;
   double pixelSpacingX;
   double pixelSpacingY;
   std::array<double, 3> imagePositionPatient;
   std::array<double, 6> imageOrientationPatient;
   std::string sopInstanceUid;
   int frameIndex;  // 多帧中的编号
   double slope;
   double intercept;


public:
  double getSlope() const;
  void setSlope(double slope);
  double getIntercept() const;
  void setIntercept(double intercept);
  int getFrameIndex() const;
  void setFrameIndex(int frameIndex);
  const std::vector<uint16_t>& getPixels() const;
  void setPixels(const std::vector<uint16_t>& pixels);

  int getRows() const;
  void setRows(int rows);

  int getCols() const;
  void setCols(int cols);

  double getPixelSpacingX() const;
  void setPixelSpacingX(double pixelSpacingX);

  double getPixelSpacingY() const;
  void setPixelSpacingY(double pixelSpacingY);

  const std::array<double, 3>& getImagePositionPatient() const;
  void setImagePositionPatient(const std::array<double, 3>& imagePositionPatient);

  const std::array<double, 6>& getImageOrientationPatient() const;
  void setImageOrientationPatient(const std::array<double, 6>& imageOrientationPatient);

  const std::string& getSopInstanceUid() const;
  void setSopInstanceUid(const std::string& sopInstanceUid);

};
