
#include "DcmtkReader.h"

DcmtkReader::DcmtkReader()
{
}

void DcmtkReader::open(const std::string& path) {
    m_fileFormat = std::make_unique<DcmFileFormat>();
    if (m_fileFormat->loadFile(path.c_str()).bad()) {
        // 处理加载失败的情况，例如抛出异常或记录错误日志
        throw std::runtime_error("Failed to load DICOM file: " + path);
    }else{
        m_dataset = std::make_unique<DcmDataset>(*m_fileFormat->getDataset());
    }
}

Series DcmtkReader::readSeries()
{
  return Series();
}

std::shared_ptr<Frame> DcmtkReader::readFrame(int index)
{
  return std::shared_ptr<Frame>();
}
