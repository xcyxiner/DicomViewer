
#pragma once
#include <algorithm>
#include <cmath>

// 窗宽窗位拖拽计算（纯函数，2D 与 MPR 渲染器共用）。
// 复刻 vtkInteractorStyleImage::WindowLevel 的计算公式——style 在
// 有观察者时不再自行应用（其 WindowLevel() 的 HasObserver 分支），
// 观察者须自行计算。两处渲染器共用本函数以保持公式单源。
// 注意：本公式忠实复刻两处原实现，未补上游的 level<0 时 dy 翻转
// （行为保持约束——修复属行为变更，另行处理）。
struct WindowLevelDrag
{
  // start/current: 拖拽起止像素坐标（style 的
  // WindowLevelStartPosition/CurrentPosition）；size: 渲染窗口像素尺寸；
  // initialWindow/initialLevel: 拖拽开始时刻的窗宽窗位（KTD11 基准值）。
  // 返回计算后的 {窗宽, 窗位}，窗宽下限 0.01。
  static void compute(const int* start,
                      const int* current,
                      const int* size,
                      double initialWindow,
                      double initialLevel,
                      double& windowOut,
                      double& levelOut)
  {
    const double window = initialWindow;
    const double level = initialLevel;

    double dx = (current[0] - start[0]) * 4.0 / size[0];
    double dy = (start[1] - current[1]) * 4.0 / size[1];
    if (std::fabs(window) > 0.01) {
      dx = dx * window;
    } else {
      dx = dx * (window < 0 ? -0.01 : 0.01);
    }
    if (std::fabs(level) > 0.01) {
      dy = dy * level;
    } else {
      dy = dy * (level < 0 ? -0.01 : 0.01);
    }
    if (window < 0.0) {
      dx = -1 * dx;
    }
    windowOut = std::max(dx + window, 0.01);
    levelOut = level - dy;
  }
};
