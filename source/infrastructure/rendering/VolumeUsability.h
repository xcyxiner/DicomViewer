
#pragma once
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

// 体绘制/MPR 共用的可用性门槛：体在 z 向至少 2 层才是真正的三维体。
// 单层体（N=1 的单文件序列）轴位尚可显示，但冠/矢面退化为 1 像素条、
// 3D 体绘制退化为单张切片，均无意义——此时进占位比进视口更诚实。
// 多帧单文件（enhanced）z 深度 >1 不受影响。
// （原为 MprPanel.cpp 匿名命名空间的文件局部 isMprUsable，抽为共享头
//   保证 MprPanel 与 VolumePanel 的门槛单一来源。）
inline bool isVolumeUsable(const vtkSmartPointer<vtkImageData>& volume)
{
  if (!volume) {
    return false;
  }
  int extent[6];
  volume->GetExtent(extent);
  return extent[5] > extent[4];
}
