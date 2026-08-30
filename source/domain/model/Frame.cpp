
#include "Frame.h"

Frame::Frame()
    : rows(0)
    , cols(0)
    , pixelSpacingX(0.0)
    , pixelSpacingY(0.0)
    , imagePositionPatient {0.0, 0.0, 0.0}
    , imageOrientationPatient {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
    , frameIndex(0)
    , slope(1.0)
    , intercept(0.0)
    , windowWidth(0.0)
    , windowCenter(0.0)
{
}

// --- Frame Getters & Setters (auto-generated) ---
const std::vector<uint16_t>& Frame::getPixels() const
{
  return pixels;
}

void Frame::setPixels(const std::vector<uint16_t>& pixels)
{
  this->pixels = pixels;
}

int Frame::getRows() const
{
  return rows;
}

void Frame::setRows(int rows)
{
  this->rows = rows;
}

int Frame::getCols() const
{
  return cols;
}

void Frame::setCols(int cols)
{
  this->cols = cols;
}

double Frame::getPixelSpacingX() const
{
  return pixelSpacingX;
}

void Frame::setPixelSpacingX(double pixelSpacingX)
{
  this->pixelSpacingX = pixelSpacingX;
}

double Frame::getPixelSpacingY() const
{
  return pixelSpacingY;
}

void Frame::setPixelSpacingY(double pixelSpacingY)
{
  this->pixelSpacingY = pixelSpacingY;
}

const std::array<double, 3>& Frame::getImagePositionPatient() const
{
  return imagePositionPatient;
}

void Frame::setImagePositionPatient(
    const std::array<double, 3>& imagePositionPatient)
{
  this->imagePositionPatient = imagePositionPatient;
}

const std::array<double, 6>& Frame::getImageOrientationPatient() const
{
  return imageOrientationPatient;
}

void Frame::setImageOrientationPatient(
    const std::array<double, 6>& imageOrientationPatient)
{
  this->imageOrientationPatient = imageOrientationPatient;
}

const std::string& Frame::getSopInstanceUid() const
{
  return sopInstanceUid;
}

void Frame::setSopInstanceUid(const std::string& sopInstanceUid)
{
  this->sopInstanceUid = sopInstanceUid;
}

// --- Frame Getters & Setters (auto-generated) ---
int Frame::getFrameIndex() const
{
  return frameIndex;
}

void Frame::setFrameIndex(int frameIndex)
{
  this->frameIndex = frameIndex;
}

// --- Frame Getters & Setters (auto-generated) ---
double Frame::getSlope() const
{
  return slope;
}

void Frame::setSlope(double slope)
{
  this->slope = slope;
}

// --- Frame Getters & Setters (auto-generated) ---
double Frame::getIntercept() const
{
  return intercept;
}

void Frame::setIntercept(double intercept)
{
  this->intercept = intercept;
}

// --- Frame Getters & Setters (auto-generated) ---
double Frame::getWindowWidth() const
{
  return windowWidth;
}

void Frame::setWindowWidth(double windowWidth)
{
  this->windowWidth = windowWidth;
}

double Frame::getWindowCenter() const
{
  return windowCenter;
}

void Frame::setWindowCenter(double windowCenter)
{
  this->windowCenter = windowCenter;
}
