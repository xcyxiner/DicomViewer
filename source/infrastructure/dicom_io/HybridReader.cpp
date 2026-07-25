
#include "HybridReader.h"
#include "DcmtkReader.h"
#include "GdcmReader.h"
HybridReader::HybridReader()
{
  m_dcmtk_reader = std::make_unique<DcmtkReader>();
  m_gdcm_reader = std::make_unique<GdcmReader>();
}

void HybridReader::open(const std::string& filePath) {
  m_dcmtk_reader->open(filePath);
  m_gdcm_reader->open(filePath);
}

Series HybridReader::readSeries(const std::string& path)
{
  return Series();
}

std::unique_ptr<Frame> HybridReader::readFrameInfo(int index)
{
  return std::unique_ptr<Frame>(new Frame());
}

IFrameCache::FramePtr HybridReader::readFrame(int index)
{
  const auto frameInfo = m_dcmtk_reader->readFrameInfo(index);
  auto frameData = m_gdcm_reader->readFrame(index);
  std::visit([&frameInfo](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
        arg->setPixelSpacingX(frameInfo->getPixelSpacingX());
        arg->setPixelSpacingY(frameInfo->getPixelSpacingY());
        arg->setImagePositionPatient(frameInfo->getImagePositionPatient());
        arg->setImageOrientationPatient(frameInfo->getImageOrientationPatient());
        arg->setSopInstanceUid(frameInfo->getSopInstanceUid());
        arg->setFrameIndex(frameInfo->getFrameIndex());
        arg->setSlope(frameInfo->getSlope());
        arg->setIntercept(frameInfo->getIntercept());
    }
  }, frameData);
  return frameData;
}

void HybridReader::close() {}
