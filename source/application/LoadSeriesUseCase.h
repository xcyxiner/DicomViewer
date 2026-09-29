
#pragma once
#include "infrastructure/dicom_io/IDicomReader.h"
#include "infrastructure/task/ITaskQueue.h"
#include <future>
#include <string>
#include "domain/model/StackDisplaySet.h"

// 后台加载结果 —— 体与显示集必须在回主线程的同一替换点一起提交
// （KTD9），失败原因随结果携带（异常禁止越过线程边界）。
struct LoadSeriesResult
{
    std::shared_ptr<StackDisplaySet> displaySet;  // 失败时为空
    vtkSmartPointer<vtkImageData> volume;         // 失败时为空
    std::string error;                            // 空表示成功
};

class LoadSeriesUseCase
{
public:
    explicit LoadSeriesUseCase();
    ~LoadSeriesUseCase()=default;

    LoadSeriesUseCase(IDicomReader& dicomReader, ITaskQueue& taskQueue);
    std::future<LoadSeriesResult> loadSeriesAsync(const std::string& path);
private:
    IDicomReader& m_dicomReader;
    ITaskQueue& m_taskQueue;
};
