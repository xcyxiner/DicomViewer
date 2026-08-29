
#pragma once
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

class ImageDataComparator
{
public:
  struct ComparisonResult
  {
    int totalPixels = 0;
    int imageWidth = 0;
    int diffCount = 0;
    double meanDiff = 0.0;
    double maxDiff = 0.0;
    int firstDiffIdx = -1;
    unsigned short firstVal1 = 0;
    unsigned short firstVal2 = 0;
    bool dimensionsMatch = true;
    bool scalarTypesMatch = true;
  };

  static ComparisonResult compare(vtkImageData* img1, vtkImageData* img2);
  static void printResult(const ComparisonResult& result);
};
