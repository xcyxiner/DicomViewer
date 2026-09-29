#pragma once

// 视图模式 —— 「View」菜单的排他三选一，决定哪些面板参与槽位分配。
// 默认 TwoDOnly：启动即只显示 2D，MPR 需经 View 菜单切出。
enum class ViewMode
{
  MprOnly,  // 仅 MPR
  TwoDOnly,  // 仅 2D（默认）
  Split  // 2D + MPR 分屏
};
