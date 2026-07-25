#include "DcmtkReader.h"
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcvrat.h>
#include <dcmtk/dcmfg/fgbase.h>
#include <dcmtk/dcmfg/fginterface.h>
#include <dcmtk/dcmfg/fgtypes.h>
DcmtkReader::DcmtkReader() {}
void DcmtkReader::open(const std::string& path)
{
  m_fileFormat = std::make_unique<DcmFileFormat>();
  if (m_fileFormat->loadFile(path.c_str()).bad()) {
    // 处理加载失败的情况，例如抛出异常或记录错误日志
    throw std::runtime_error("Failed to load DICOM file: " + path);
  } else {
    m_dataset = std::make_unique<DcmDataset>(*m_fileFormat->getDataset());
  }
}
Series DcmtkReader::readSeries(const std::string& path)
{
  return Series();
}
template<size_t N>
bool getTagAsArray(const std::unique_ptr<DcmDataset>& dataset,
                   const DcmTagKey& tag,
                   std::array<double, N>& arr)
{
  bool allGood = true;
  for (size_t i = 0; i < N; ++i) {
    OFCondition status = dataset->findAndGetFloat64(tag, arr[i], i);
    if (!status.good()) {
      allGood = false;
      // 可以选择break或继续
      throw std::runtime_error("Failed to read DICOM tag: " + std::string(status.text()));
    }
  }
  return allGood;
}
std::unique_ptr<Frame> DcmtkReader::readFrameInfo(int index)
{
  auto frame =std::make_unique<Frame>();
  OFString sopInstanceUid;
  m_dataset->findAndGetOFString(DCM_SOPInstanceUID, sopInstanceUid);
  frame->setSopInstanceUid(std::string(sopInstanceUid.c_str()));
  OFString numberOfFramesStr;
  int numberOfFrames = 1;
  if (m_dataset->findAndGetOFString(DCM_NumberOfFrames, numberOfFramesStr)
          .good())
  {
    numberOfFrames = std::atoi(numberOfFramesStr.c_str());
  }
  //更新索引
  frame->setFrameIndex(index);
  bool isSingleFrame = (numberOfFrames == 1);
  bool isEnhancedMultiFrame = false;  // 这里可以添加逻辑来判断是否为增强型多帧DICOM
  bool isOldMultiFrame = false;  // 这里可以添加逻辑来判断是否为旧式多帧DICOM
  if (!isSingleFrame) {
    FGInterface fgInterface;
    if (m_dataset) {
      if (fgInterface.read(*m_dataset).good()) {
        isEnhancedMultiFrame = true;
      } else {
        isOldMultiFrame = true;
      }
    }
  }
  //先处理单帧
  if (isSingleFrame) {
    // 像素间距 先行 后列
    std::array<double, 2> pixelSpacing = {0.0, 0.0};
    if (getTagAsArray(m_dataset, DCM_PixelSpacing, pixelSpacing)) {
      frame->setPixelSpacingY(pixelSpacing[0]);
      frame->setPixelSpacingX(pixelSpacing[1]);
    }
    // 图像第一个像素的3D世界坐标
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    if (getTagAsArray(m_dataset, DCM_ImagePositionPatient, position)) {
      frame->setImagePositionPatient(position);
    }
    // 图像行和列方向的方向余弦，包含6个值。
    std::array<double, 6> orientation = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    if (getTagAsArray(m_dataset, DCM_ImageOrientationPatient, orientation)) {
      frame->setImageOrientationPatient(orientation);
    }
    // 斜率和截距
    double slope = 1.0;
    double intercept = 0.0;
    OFCondition status =  m_dataset->findAndGetFloat64(DCM_RescaleSlope, slope, 0);
    if(!status.good()) {
      slope = 1.0; // 默认值
    }
    status =  m_dataset->findAndGetFloat64(DCM_RescaleIntercept, intercept, 0);
    if(!status.good()) {
      intercept = 0.0; // 默认值
    }
    frame->setSlope(slope);
    frame->setIntercept(intercept);
  }
  return frame;
}
IFrameCache::FramePtr DcmtkReader::readFrame(int index)
{
  return IFrameCache::FramePtr();
}

void DcmtkReader::close() {}
