#include <cmath>
#include <iostream>

#include "ImageDataComparator.h"

ImageDataComparator::ComparisonResult ImageDataComparator::compare(
    vtkImageData* img1, vtkImageData* img2)
{
  ComparisonResult result;

  int dims1[3], dims2[3];
  img1->GetDimensions(dims1);
  img2->GetDimensions(dims2);
  if (dims1[0] != dims2[0] || dims1[1] != dims2[1] || dims1[2] != dims2[2]) {
    result.dimensionsMatch = false;
    return result;
  }

  int scalarType1 = img1->GetScalarType();
  int scalarType2 = img2->GetScalarType();
  if (scalarType1 != scalarType2) {
    result.scalarTypesMatch = false;
    return result;
  }

  auto* ptr1 = static_cast<unsigned short*>(img1->GetScalarPointer());
  auto* ptr2 = static_cast<unsigned short*>(img2->GetScalarPointer());
  int totalPixels = dims1[0] * dims1[1] * dims1[2];
  result.totalPixels = totalPixels;
  result.imageWidth = dims1[0];

  double sumDiff = 0.0;
  double maxDiff = 0.0;
  int diffCount = 0;
  int firstDiffIdx = -1;
  unsigned short firstVal1 = 0, firstVal2 = 0;

  for (int i = 0; i < totalPixels; ++i) {
    unsigned short v1 = ptr1[i];
    unsigned short v2 = ptr2[i];
    double diff = std::abs(static_cast<double>(v1) - static_cast<double>(v2));
    if (diff >= 0.0) {
      sumDiff += diff;
      if (diff > maxDiff) {
        maxDiff = diff;
      }
      diffCount++;
      if (firstDiffIdx == -1) {
        firstDiffIdx = i;
        firstVal1 = v1;
        firstVal2 = v2;
      }
    }
  }

  result.diffCount = diffCount;
  result.meanDiff = (diffCount > 0) ? (sumDiff / diffCount) : 0.0;
  result.maxDiff = maxDiff;
  result.firstDiffIdx = firstDiffIdx;
  result.firstVal1 = firstVal1;
  result.firstVal2 = firstVal2;
  return result;
}

void ImageDataComparator::printResult(const ComparisonResult& result)
{
  if (!result.dimensionsMatch) {
    std::cerr << "Dimensions do not match!" << std::endl;
    return;
  }
  if (!result.scalarTypesMatch) {
    std::cerr << "Scalar types differ!" << std::endl;
    return;
  }
  std::cout << "=== Pixel Comparison ===" << std::endl;
  std::cout << "Total pixels: " << result.totalPixels << std::endl;
  std::cout << "Number of differing pixels: " << result.diffCount << std::endl;
  std::cout << "Mean absolute difference: " << result.meanDiff << std::endl;
  std::cout << "Max absolute difference: " << result.maxDiff << std::endl;
  if (result.firstDiffIdx != -1) {
    std::cout << "First diff at linear index " << result.firstDiffIdx
              << " (row " << result.firstDiffIdx / result.imageWidth << ", col "
              << result.firstDiffIdx % result.imageWidth << ")"
              << ": img1=" << result.firstVal1 << ", img2=" << result.firstVal2
              << std::endl;
  } else {
    std::cout << "All pixels are identical (within numeric precision)."
              << std::endl;
  }
}
