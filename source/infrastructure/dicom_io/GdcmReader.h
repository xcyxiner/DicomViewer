
#pragma once
#include "IDicomReader.h"
#include <gdcmImageReader.h>
#include <gdcmImageRegionReader.h>
class GdcmReader: public IDicomReader 
{
public:
    explicit GdcmReader();
    ~GdcmReader()=default;

public:
    void open(const std::string& filePath) override;
    Series readSeries(const std::string& path) override;
    std::unique_ptr<Frame> readFrameInfo( int index) override;
    IFrameCache::FramePtr readFrame(int index) override;
    void close() override;
private:
    std::unique_ptr<gdcm::ImageReader> m_image_reader;
    std::unique_ptr<gdcm::ImageRegionReader> m_image_region_reader;
    const char* filePath = nullptr;
    std::vector<uint16_t> extractPixelData(const gdcm::Image& image, int frameIndex);
};

