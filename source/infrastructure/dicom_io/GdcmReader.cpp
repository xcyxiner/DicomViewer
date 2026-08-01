

#include "GdcmReader.h"

#include <gdcmBoxRegion.h>
#include <gdcmImage.h>
#include <gdcmImageHelper.h>
#include <gdcmPixelFormat.h>
#include <gdcmPixmap.h>

GdcmReader::GdcmReader() {}

void GdcmReader::open(const std::string& path)
{
  m_image_reader = std::make_unique<gdcm::ImageReader>();
  m_image_reader->SetFileName(path.c_str());
  filePath = path.c_str();
  if (!m_image_reader->Read()) {
    throw std::runtime_error("Failed to read DICOM file: " + path);
  }
}

Series GdcmReader::readSeries(const std::string& path)
{
  return Series();
}

IFrameCache::FramePtr GdcmReader::readFrame(int index)
{
  if (!m_image_reader) {
    throw std::runtime_error("DICOM file not opened. Call open() first.");
  }
  if (!filePath) {
    throw std::runtime_error("File path is not set. Call open() first.");
  }
  m_image_reader->SetFileName(filePath);
  if (!m_image_reader->Read()) {
    throw std::runtime_error("Failed to read DICOM file: "
                             + std::string(filePath));
  }
  auto image = m_image_reader->GetImage();
  auto dims = image.GetDimensions();
  auto numFrames = (image.GetNumberOfDimensions() == 3) ? dims[2] : 1;
  if (index < 0 || index >= static_cast<int>(numFrames)) {
    throw std::out_of_range("Frame index out of range");
  }
  unsigned int rows = image.GetRows();
  unsigned int cols = image.GetColumns();
  std::vector<uint16_t> pixels = extractPixelData(image, index);
  auto frame = std::make_unique<Frame>();
  frame->setPixels(pixels);
  frame->setRows(rows);
  frame->setCols(cols);
  return frame;
}

void GdcmReader::close() {}

template<typename T>
std::vector<uint16_t> convertPixels(const char* buffer, size_t count)
{
  const T* src = reinterpret_cast<const T*>(buffer);
  std::vector<uint16_t> pixels(count);
  for (size_t i = 0; i < count; ++i) {
    if constexpr (std::is_same_v<T, int8_t>) {
      // [-128,127] → [0,255]
      pixels[i] = static_cast<uint16_t>(static_cast<int16_t>(src[i]) + 128);
    } else if constexpr (std::is_same_v<T, int16_t>) {
      // 直接按位 reinterpret（DICOM 正确做法）
      pixels[i] = static_cast<uint16_t>(src[i]);
    } else if constexpr (std::is_same_v<T, uint8_t>) {
      // 8-bit → 16-bit（无符号扩展）
      pixels[i] = static_cast<uint16_t>(src[i]) * 257u;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      pixels[i] = src[i];
    } else if constexpr (std::is_floating_point_v<T>) {
      // 假设 float 在 [0,1]
      constexpr double scale = 65535.0;
      double v = src[i];
      v = std::clamp(v, 0.0, 1.0);
      pixels[i] = static_cast<uint16_t>(v * scale + 0.5);
    } else {
      static_assert(!sizeof(T), "Unsupported pixel type");
    }
  }
  return pixels;
}

// 运行时分派：枚举 -> 调用对应模板实例
std::vector<uint16_t> GdcmReader::extractPixelData(const gdcm::Image& image,
                                                   int frameIndex)
{
  const gdcm::PixelFormat& pf = image.GetPixelFormat();
  unsigned int rows = image.GetRows();
  unsigned int cols = image.GetColumns();
  size_t count = rows * cols;
  // 计算帧偏移
  const unsigned int* dims = image.GetDimensions();
  unsigned int numFrames = (image.GetNumberOfDimensions() == 3) ? dims[2] : 1;
  size_t frameSizeInBytes = image.GetBufferLength() / numFrames;
  unsigned long bufLen = image.GetBufferLength();
  std::vector<char> fullBuffer(bufLen);
  image.GetBuffer(fullBuffer.data());
  const char* buffer = fullBuffer.data() + frameIndex * frameSizeInBytes;
  switch (pf.GetScalarType()) {
    case gdcm::PixelFormat::UINT8:
      return convertPixels<uint8_t>(buffer, count);
    case gdcm::PixelFormat::INT8:
      return convertPixels<int8_t>(buffer, count);
    case gdcm::PixelFormat::UINT16:
      return convertPixels<uint16_t>(buffer, count);
    case gdcm::PixelFormat::INT16:
      return convertPixels<int16_t>(buffer, count);
    default:
      throw std::runtime_error("Unsupported pixel type");
  }
}

std::unique_ptr<Frame> GdcmReader::readFrameInfo(int index)
{
  return std::unique_ptr<Frame>(new Frame());
}
