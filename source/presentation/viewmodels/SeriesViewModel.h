
#pragma once
#include <QObject>
#include <QString>
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

    // 按当前层从体抽层渲染（体经 getVolume 按 seriesKey 从缓存取；
    // 抽层复用 ImageSliceExtractor::extractSlice —— 与 IDicomReader::
    // readFrame 共同委托的同一实现，GUI 不触碰 reader（KTD9））。
    void render();

    // 异步加载序列。处理流程：join 上一次加载（串行化，兼作 reader
    // 单例免于线程池并发的屏障）→ use case 后台执行 → 成功回主线程
    // 同一替换点提交体与 StackDisplaySet 后发 seriesLoaded；
    // 失败发 loadFailed，不触碰旧状态（已显示图像保留）。
    void loadSeries(const std::string& path);

    void fitToWindow();

    // 恢复到加载时的初始窗宽窗位（菜单 reset 路径），经同一广播链。
    void resetWindowLevel();

    // 设置窗宽窗位并广播给所有面板（windowLevelChanged，最后操作为准）。
    // 与 notifyWindowLevelInteracted 走同一通道；值与当前显示设置
    // 相同时为幂等空操作（KTD11 值比较防环）。
    void setWindowLevel(double windowWidth, double windowCenter);

    // 交互回灌入口（KTD11）：2D/MPR renderer 的窗宽窗位交互回调统一
    // 回到这里，与 setWindowLevel 同一广播链（更新显示设置 → 写 2D
    // renderer → emit windowLevelChanged 恰一次），最后操作为准。
    void notifyWindowLevelInteracted(double windowWidth,
                                     double windowCenter);

    // ---- 2D 逐层导航 ----
    // 处理流程：夹取层索引 → 改 StackDisplaySet::currentIndex →
    // 发 sliceChanged → 重绘当前层；不触发 imageChanged（不重置相机）。
    // 同值调用为幂等空操作（不重发 sliceChanged，滑条同步依赖此性质）；
    // 未加载（N=0）时为安全空操作。
    void nextFrame();
    void previousFrame();
    void setCurrentIndex(int index);

    // 当前序列总层数（未加载返回 0）与当前层索引（未加载返回 0）——
    // 供侧边滑条 range 与 n/N 层号显示同步（R4）。
    int frameCount() const;
    int currentIndex() const;

    // 取体入口（MPR 面板经此按当前序列的 seriesKey 取整幅体，
    // 不直接读 IFrameCache）。未加载时返回空指针。
    vtkSmartPointer<vtkImageData> getVolume() const;

signals:
    // 序列加载完成（携带 seriesKey）。MPR 面板订阅后经 getVolume 取体。
    void seriesLoaded(const QString& seriesKey);
    // 加载失败（路径 + 原因）。呈现界面由面板订阅（见 U3）。
    void loadFailed(const QString& path, const QString& reason);
    // 翻层（新层索引）。MPR 面板订阅后同步切面。
    void sliceChanged(int index);
    // 窗宽窗位变更广播（菜单 / 2D 拖动 / MPR 拖动统一经此，最后操作为准）。
    void windowLevelChanged(double windowWidth, double windowCenter);
    // 序列数据就绪可渲染（加载成功后发一次，面板据此 fitToWindow）。
    void imageChanged();

private:
    // 窗宽窗位广播核心（setWindowLevel / notifyWindowLevelInteracted
    // 共用）：空值守卫 → 值比较幂等 → 持久化到 StackDisplaySet
    // （翻层重绘沿用）→ 写 2D renderer（内部抑制回调）→ 恰发一次
    // windowLevelChanged。
    void applyWindowLevel(double windowWidth, double windowCenter);

    // 失败通道统一出口：队列投递到主线程后发 loadFailed（成功分支与
    // 两条异常防御分支共用，恰发一次）。
    void postLoadFailed(const std::string& path, const std::string& reason);

private:
    std::shared_ptr<IImageRenderer> m_imageRenderer;
    std::shared_ptr<StackDisplaySet> m_stackDisplaySet;
    std::shared_ptr<IDicomReader> m_dicomReader;
    std::shared_ptr<ITaskQueue> m_taskQueue;
    std::shared_ptr<IFrameCache> m_frameCache;
    std::unique_ptr<LoadSeriesUseCase> m_loadSeriesUseCase;
    std::thread m_workerThread;
    // 加载时捕获的初始窗宽窗位（reset 路径的目标值；不随后续修改变
    // 化）。缺省值以领域对象 DisplaySettings 为单一事实源（加载成功
    // 后会被第 0 帧实际值覆盖）。
    double m_initialWindowWidth = DisplaySettings {}.getWindowWidth();
    double m_initialWindowCenter = DisplaySettings {}.getWindowCenter();
};
