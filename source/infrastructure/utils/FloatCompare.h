
#pragma once
#include <algorithm>
#include <cmath>

// 浮点同值判断（-Werror=float-equal 禁止 ==）。
// 用于窗宽窗位「同值早退」链路：判定值无实质变化即可，
// 取相对 + 绝对混合容差，量级无关（窗宽可达数千 HU）。
struct FloatCompare
{
  static bool nearlyEqual(double a, double b)
  {
    const double scale =
        std::max({1.0, std::fabs(a), std::fabs(b)});
    return std::fabs(a - b) <= 1e-9 * scale;
  }
};
