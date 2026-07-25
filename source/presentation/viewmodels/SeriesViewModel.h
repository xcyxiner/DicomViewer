
#pragma once
#include <QObject>
#include <memory>
#include <string>
#include <future>
#include "infrastructure/rendering/IImageRenderer.h"
#include "infrastructure/dicom_io/IDicomReader.h"
#include "application/LoadSeriesUseCase.h"
#include "infrastructure/task/ITaskQueue.h"
#include "infrastructure/cache/IFrameCache.h"
class SeriesViewModel : public QObject
{
    Q_OBJECT
public:
    explicit SeriesViewModel(QObject *parent = nullptr);

 public:
    void setRenderWindow(vtkSmartPointer<vtkRenderWindow> window);
    void render();
    void loadSeries(const std::string& path);

signals:
    void imageChanged();

private:
    std::shared_ptr<IImageRenderer> m_imageRenderer;
    std::shared_ptr<StackDisplaySet> m_stackDisplaySet;
    std::unique_ptr<IDicomReader> m_dicomReader;
    std::unique_ptr<ITaskQueue> m_taskQueue;
    std::unique_ptr<IFrameCache> m_frameCache;
    std::unique_ptr<LoadSeriesUseCase> m_loadSeriesUseCase;   
};
