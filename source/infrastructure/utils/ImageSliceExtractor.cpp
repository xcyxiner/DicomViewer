
#include <cstring>

#include "ImageSliceExtractor.h"

#include <vtkAbstractArray.h>

vtkSmartPointer<vtkImageData> ImageSliceExtractor::extractSlice(
    vtkImageData* volume, int index)
{
  if (!volume) {
    return nullptr;
  }

  int extent[6];
  volume->GetExtent(extent);
  if (extent[5] < extent[4] || index < extent[4] || index > extent[5]) {
    return nullptr;
  }

  // 几何原样携带：抽层与体（及 MPR reslice）消费同一份患者坐标
  // （origin/direction 为 KTD6 烘入后的患者空间）
  auto slice = vtkSmartPointer<vtkImageData>::New();
  slice->SetExtent(extent[0], extent[1], extent[2], extent[3], index, index);
  slice->SetSpacing(volume->GetSpacing());
  slice->SetOrigin(volume->GetOrigin());
  slice->SetDirectionMatrix(volume->GetDirectionMatrix());
  slice->AllocateScalars(volume->GetScalarType(),
                         volume->GetNumberOfScalarComponents());

  // 同一 z 平面的点数据在内存中连续（x 最快），整块拷贝
  const vtkIdType planeBytes = static_cast<vtkIdType>(extent[1] - extent[0] + 1)
      * static_cast<vtkIdType>(extent[3] - extent[2] + 1)
      * static_cast<vtkIdType>(volume->GetNumberOfScalarComponents())
      * static_cast<vtkIdType>(vtkAbstractArray::GetDataTypeSize(
          volume->GetScalarType()));

  void* dst = slice->GetScalarPointer(extent[0], extent[2], index);
  const void* src = volume->GetScalarPointer(extent[0], extent[2], index);
  if (!dst || !src) {
    return nullptr;
  }
  std::memcpy(dst, src, static_cast<size_t>(planeBytes));
  slice->Modified();

  return slice;
}
