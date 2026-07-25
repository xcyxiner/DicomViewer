
#pragma once
#include "IDicomReader.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdatset.h"

class DcmtkReader : public IDicomReader
{
public:
    explicit DcmtkReader();
    ~DcmtkReader()=default;
    virtual void open(const std::string& filePath) override;
    virtual Series readSeries(const std::string& path) override;
    virtual std::unique_ptr<Frame> readFrameInfo(int frameIndex) override;
    virtual IFrameCache::FramePtr readFrame(int frameIndex) override;
    virtual void close() override;   
private:
    // 这里可以添加Dcmtk相关的成员变量，例如DcmFileFormat对象等
   std::unique_ptr<DcmFileFormat> m_fileFormat; // 示例成员变量，实际根据Dcmtk的使用情况调整
   std::unique_ptr<DcmDataset> m_dataset; // 示例成员变量，实际根据Dcmtk的使用情况调整
};
