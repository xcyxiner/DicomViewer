
#pragma once
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

// 从整幅体抽取单层（extent z = index）—— 全计划唯一的抽层实现
// （KTD1/KTD6）：
//  - VTKDicomAdaptReader::readFrame 委托此处取像素，再挂逐层
//    FieldData 元数据；
//  - SeriesViewModel::render 也经此处对体快照抽层渲染 —— GUI 侧
//    不触碰 reader 单例（KTD9 加载期隔离），但抽层逻辑仍只此一份。
// 抽出的层携带原体的 spacing/origin/direction，患者几何原样传递，
// 2D 与 MPR 消费同一几何。越界或输入为空返回空指针。
class ImageSliceExtractor
{
public:
  static vtkSmartPointer<vtkImageData> extractSlice(vtkImageData* volume,
                                                    int index);
};
