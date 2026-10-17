#pragma once

// 视图模式 —— 「View」菜单的排他二选一，决定哪些面板参与槽位分配。
// 默认 TwoDOnly：启动即只显示 2D；MPR+3D 需经 View 菜单切出，
// 进入时自动切 2x2 Grid、切回仅 2D 时回 Single（GUIWindow::setViewMode）。
enum class ViewMode
{
  TwoDOnly,  // 仅 2D（默认）
  Mpr3d  // MPR 三平面 + 3D 体绘制（四格）
};
