
#pragma once
#include "infrastructure/dicom_io/IDicomReader.h"
#include "infrastructure/task/ITaskQueue.h"
#include "infrastructure/cache/IFrameCache.h"
#include <future>
#include "domain/model/StackDisplaySet.h"
class LoadSeriesUseCase 
{
public:
    explicit LoadSeriesUseCase();
    ~LoadSeriesUseCase()=default;

    LoadSeriesUseCase(IDicomReader& dicomReader,
                      ITaskQueue& taskQueue,
                      IFrameCache& frameCache);
    std::future<std::shared_ptr<StackDisplaySet>> loadSeriesAsync(const std::string& path);
private:
    // Add private members and methods here
    IDicomReader& m_dicomReader;
    ITaskQueue& m_taskQueue;
    IFrameCache& m_frameCache;
};
