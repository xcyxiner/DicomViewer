
#pragma once
#include <QObject>
#include <memory>
#include <string>
#include <future>
#include <thread>
#include "infrastructure/rendering/IImageRenderer.h"
#include "infrastructure/dicom_io/IDicomReader.h"
#include "application/LoadSeriesUseCase.h"
#include "infrastructure/task/ITaskQueue.h"
#include "infrastructure/cache/IFrameCache.h"

class SeriesViewModel : public QObject
{
    Q_OBJECT
public:
    explicit SeriesViewModel(std::shared_ptr<IImageRenderer> renderer,
                             std::shared_ptr<IDicomReader> reader,
                             std::shared_ptr<ITaskQueue> taskQueue,
                             std::shared_ptr<IFrameCache> frameCache,
                             QObject *parent = nullptr);
    ~SeriesViewModel() override;

 public:
    void setRenderWindow(vtkSmartPointer<vtkRenderWindow> window);
    void render();
    void loadSeries(const std::string& path);
    void fitToWindow();

signals:
    void imageChanged();

private:
    std::shared_ptr<IImageRenderer> m_imageRenderer;
    std::shared_ptr<StackDisplaySet> m_stackDisplaySet;
    std::shared_ptr<IDicomReader> m_dicomReader;
    std::shared_ptr<ITaskQueue> m_taskQueue;
    std::shared_ptr<IFrameCache> m_frameCache;
    std::unique_ptr<LoadSeriesUseCase> m_loadSeriesUseCase;
    std::thread m_workerThread;
};
