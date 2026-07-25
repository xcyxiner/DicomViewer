
#pragma once
#include "IDicomReader.h"
class HybridReader : public IDicomReader
{
public:
    explicit HybridReader();
    ~HybridReader()=default;
public:
    void open(const std::string& filePath) override;
    Series readSeries(const std::string& path) override;
    std::unique_ptr<Frame> readFrameInfo(int index) override;
    IFrameCache::FramePtr readFrame(int index) override;
    void close() override;
private:
    std::unique_ptr<IDicomReader> m_dcmtk_reader;
    std::unique_ptr<IDicomReader> m_gdcm_reader;
};
