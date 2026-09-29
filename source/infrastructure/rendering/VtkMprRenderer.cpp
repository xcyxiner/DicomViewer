#include <algorithm>
#include <cmath>

#include "VtkMprRenderer.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkMath.h>
#include <vtkMatrix3x3.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include "infrastructure/utils/FloatCompare.h"
#include "infrastructure/utils/WindowLevelDrag.h"

namespace
{

// pane(轴/冠/矢) → 数据轴索引（vtkImageViewer2 切片方向枚举语义：
// YZ=0 → x，XZ=1 → y，XY=2 → z）。
// 轴位=Z(2)，冠状=Y(1)，矢状=X(0)。
// （计划 U5 文本"SetSliceOrientation 0/1/2=轴冠矢"与 VTK 枚举语义
//   相反，此处按枚举语义映射以保证 R5 三平面显示正确。）
constexpr int kPlaneAxis[3] = {2, 1, 0};

// 滚轮翻层样式（KTD10）：vtkInteractorStyleImage 未覆写滚轮，缺省继承
// trackball 的相机推进（dolly）——此处覆写为切层回调；修饰键
// （Shift/Ctrl/Alt）让行给基类做推进，复刻 vtkResliceImageViewer
// ScrollCallback 的让行语义。回调在 VtkMprRenderer 构造时按平面绑定；
// 样式随渲染器成员同生命周期，窗口重绑经 SetInteractorStyle 自动装卸
// （换样式时 vtkRenderWindowInteractor 会卸除旧样式在 interactor 上的
// 观察者），无需挂接/卸载簿记。
class MprImageStyle : public vtkInteractorStyleImage
{
public:
  static MprImageStyle* New();
  vtkTypeMacro(MprImageStyle, vtkInteractorStyleImage);

  void OnMouseWheelForward() override
  {
    if (modifierHeld()) {
      this->Superclass::OnMouseWheelForward();
      return;
    }
    if (wheelHandler) {
      wheelHandler(+1);  // 滚轮前进 = 索引 +1（与旧 IncrementSlice 同向）
    }
  }

  void OnMouseWheelBackward() override
  {
    if (modifierHeld()) {
      this->Superclass::OnMouseWheelBackward();
      return;
    }
    if (wheelHandler) {
      wheelHandler(-1);
    }
  }

  // 由 VtkMprRenderer 构造时按平面绑定的翻层入口
  std::function<void(int)> wheelHandler;

protected:
  MprImageStyle() = default;
  ~MprImageStyle() override = default;

private:
  MprImageStyle(const MprImageStyle&) = delete;
  void operator=(const MprImageStyle&) = delete;

  bool modifierHeld() const
  {
    return this->Interactor
        && (this->Interactor->GetShiftKey() || this->Interactor->GetControlKey()
            || this->Interactor->GetAltKey());
  }
};

vtkStandardNewMacro(MprImageStyle);

}  // namespace

VtkMprRenderer::VtkMprRenderer()
{
  for (int plane = 0; plane < kPlaneCount; ++plane) {
    // 每子视口一条 ImageSlice → ImageResliceMapper 管线（黑底 +
    // 平行投影，与旧 viewer 的黑底/SetupInteractor 行为一致）
    m_renderers[plane] = vtkSmartPointer<vtkRenderer>::New();
    m_renderers[plane]->SetBackground(0.0, 0.0, 0.0);
    m_renderers[plane]->GetActiveCamera()->ParallelProjectionOn();

    // SliceFacesCamera/SliceAtFocalPoint 默认关闭（vtkImageMapper3D
    // 构造），mapper 走显式 SetSlicePlane 的世界坐标切面
    m_mappers[plane] = vtkSmartPointer<vtkImageResliceMapper>::New();

    m_slices[plane] = vtkSmartPointer<vtkImageSlice>::New();
    m_slices[plane]->SetMapper(m_mappers[plane]);
    m_renderers[plane]->AddViewProp(m_slices[plane]);

    m_planes[plane] = vtkSmartPointer<vtkPlane>::New();
    m_mappers[plane]->SetSlicePlane(m_planes[plane]);
  }

  // ---- U6 联动观察者（KTD11 / R8）----
  // 每子视口一个交互样式：左键拖拽 → style 的 StartWindowLevelEvent/
  // WindowLevelEvent → 本对象计算并应用后回灌；滚轮经样式子类覆写为
  // 翻层（见 handleSliceWheel）。
  for (int plane = 0; plane < kPlaneCount; ++plane) {
    auto style = vtkSmartPointer<MprImageStyle>::New();
    style->wheelHandler = [this, plane](int sign)
    { handleSliceWheel(plane, sign); };
    style->AddObserver(vtkCommand::StartWindowLevelEvent,
                       this,
                       &VtkMprRenderer::handleStartWindowLevel);
    style->AddObserver(
        vtkCommand::WindowLevelEvent, this, &VtkMprRenderer::handleWindowLevel);
    m_styles[plane] = style;
  }
}

VtkMprRenderer::~VtkMprRenderer() = default;

void VtkMprRenderer::setRenderTarget(vtkSmartPointer<vtkRenderWindow> window,
                                     int plane)
{
  if (!window || plane < 0 || plane >= kPlaneCount) {
    return;
  }
  if (m_windows[plane] != window) {
    // 换窗（或首次注入）：从旧窗摘除本平面 renderer 后挂到新窗
    // （renderer 的 RenderWindow 随之重链）
    if (m_windows[plane]) {
      m_windows[plane]->RemoveRenderer(m_renderers[plane]);
    }
    window->AddRenderer(m_renderers[plane]);
    m_windows[plane] = window;
    // 绑定变更才恢复相机（首次绑定/换窗）且已有体 → 朝向 + fit；
    // 同窗重入跳过，保住用户平移缩放。refreshLayout 随后的
    // fitToWindow 仍会兜底，此处保证"先有体后激活"也能直接显示。
    if (m_volume) {
      updateCameraOrientation(plane);
      m_renderers[plane]->ResetCamera();
    }
  }
  if (auto* interactor = window->GetInteractor()) {
    // 装配本子视口的交互样式（幂等：同样式重复 SetInteractorStyle
    // 由 vtk 内部早退；换样式时旧样式的 interactor 观察者自动卸除）
    interactor->SetInteractorStyle(m_styles[plane]);
  }
}

void VtkMprRenderer::setVolume(vtkSmartPointer<vtkImageData> volume)
{
  if (!volume) {
    // 无体 no-op：占位由面板层处理（KTD12/指令），不崩溃
    return;
  }
  if (volume == m_volume) {
    // 同一体重复推送（activate 补推）：保留切面与窗宽窗位，仅补绘
    // （m_volume 只在实际应用时赋值，同指针即已应用过）
    render();
    return;
  }
  m_volume = volume;

  int extent[6];
  volume->GetExtent(extent);
  double range[2];
  volume->GetScalarRange(range);

  for (int plane = 0; plane < kPlaneCount; ++plane) {
    m_mappers[plane]->SetInputData(volume);

    // 该体标量范围作为默认窗宽窗位（旧 SetInputData 的重置语义）：
    // 只写 property、不写缓存——缓存仍只由 updateWindowLevel 写入，
    // 应用后若有缓存再统一恢复
    auto* property = m_slices[plane]->GetProperty();
    property->SetColorWindow(range[1] - range[0]);
    property->SetColorLevel(0.5 * (range[0] + range[1]));

    // 默认切片居中（与旧 GetSliceRange 中值语义一致；轴位面板随后
    // 会由 MprPanel 推 2D 当前层覆盖）
    const int axis = kPlaneAxis[plane];
    m_sliceIndex[plane] = (extent[2 * axis] + extent[2 * axis + 1]) / 2;
    applySlice(plane);

    updateCameraOrientation(plane);
    if (m_windows[plane]) {
      // 新体新几何：已绑定子视口重新 fit（行为差异——旧实现保留旧
      // 相机；换序列时新体居中更合理，已在计划取舍中注明）
      m_renderers[plane]->ResetCamera();
    }
  }

  if (m_hasWindowLevel) {
    for (int plane = 0; plane < kPlaneCount; ++plane) {
      applyWindowLevel(plane);
    }
  }
  render();
}

void VtkMprRenderer::updateWindowLevel(double windowWidth, double windowCenter)
{
  // 同值早退：拖拽链路（本对象应用 → ViewModel 广播回灌）第二道
  // 写入值相同，跳过三联应用 + 重绘消除每帧二次 Render（E1）。
  // GL 重建后的重绘由 MprPanel::activate 末尾的显式 render() 保障；
  // setVolume 重置标量范围后的恢复由 setVolume 自身的 applyWindowLevel
  // 完成，不经本早退路径。
  if (m_hasWindowLevel && FloatCompare::nearlyEqual(m_windowWidth, windowWidth)
      && FloatCompare::nearlyEqual(m_windowCenter, windowCenter))
  {
    return;
  }
  // 程序化回写期间抑制交互回灌（KTD11 第一道闸；实际交互事件不会在
  // 本调用栈内触发，此标志为纵深防御，第二道闸在 ViewModel 值比较）
  m_suppressWindowLevelCallback = true;
  m_windowWidth = windowWidth;
  m_windowCenter = windowCenter;
  m_hasWindowLevel = true;
  for (int plane = 0; plane < kPlaneCount; ++plane) {
    applyWindowLevel(plane);
  }
  render();
  m_suppressWindowLevelCallback = false;
}

void VtkMprRenderer::applyWindowLevel(int plane)
{
  // 各平面的 vtkImageProperty 独立持有窗宽窗位（无共享 LUT，三平面
  // 写入同值即等价旧共享 LUT 的同步）；先 window 后 level 顺序沿用
  // 旧实现；无输入亦安全（只触碰 property）。
  auto* property = m_slices[plane]->GetProperty();
  property->SetColorWindow(m_windowWidth);
  property->SetColorLevel(m_windowCenter);
}

void VtkMprRenderer::applySlice(int plane)
{
  if (!m_volume || plane < 0 || plane >= kPlaneCount) {
    return;
  }
  int extent[6];
  m_volume->GetExtent(extent);
  const int axis = kPlaneAxis[plane];
  // 另两轴取 extent 最小角点：只要本轴索引一致即定义同一张切面
  // （患者矩阵为正交旋转，角点选择不影响切面归属）
  int ijk[3] = {extent[0], extent[2], extent[4]};
  ijk[axis] = m_sliceIndex[plane];
  double world[3] = {0.0, 0.0, 0.0};
  m_volume->TransformIndexToPhysicalPoint(ijk[0], ijk[1], ijk[2], world);
  m_planes[plane]->SetOrigin(world);

  // normal = 方向矩阵第 axis 列（患者空间），归一化以防非单位输入。
  // 每次重设幂等（plane MTime 变更即触发 mapper 重切）；法向符号无
  // 关显示——mapper 的 UpdateResliceMatrix 会把法向翻向相机。
  auto* direction = m_volume->GetDirectionMatrix();
  double normal[3] = {direction->GetElement(0, axis),
                      direction->GetElement(1, axis),
                      direction->GetElement(2, axis)};
  vtkMath::Normalize(normal);
  m_planes[plane]->SetNormal(normal);

  if (m_windows[plane]) {
    // 切面移动后同步裁剪面（复刻 vtkImageViewer2::UpdateDisplayExtent
    // 的裁剪调整），避免翻层后图像被裁掉
    m_renderers[plane]->ResetCameraClippingRange();
  }
}

void VtkMprRenderer::updateCameraOrientation(int plane)
{
  if (!m_volume || plane < 0 || plane >= kPlaneCount) {
    return;
  }
  auto* camera = m_renderers[plane]->GetActiveCamera();
  auto* direction = m_volume->GetDirectionMatrix();
  double d0[3];
  double d1[3];
  double d2[3];
  for (int row = 0; row < 3; ++row) {
    d0[row] = direction->GetElement(row, 0);
    d1[row] = direction->GetElement(row, 1);
    d2[row] = direction->GetElement(row, 2);
  }
  // 语义复刻 vtkImageViewer2::UpdateOrientation，但用方向矩阵列把
  // 数据轴映射到患者空间：轴位沿 +k 看（viewup +j）、冠状沿 −j 看
  // （viewup +k）、矢状沿 +i 看（viewup +k）。focal 先置原点，
  // 随后 ResetCamera 移到包围盒中心（与 UpdateOrientation 的单位
  // 向量写法同构）。
  double position[3];
  double viewUp[3];
  switch (plane) {
    case IMprRenderer::kPlaneAxial:
      position[0] = d2[0];
      position[1] = d2[1];
      position[2] = d2[2];
      viewUp[0] = d1[0];
      viewUp[1] = d1[1];
      viewUp[2] = d1[2];
      break;
    case IMprRenderer::kPlaneCoronal:
      position[0] = -d1[0];
      position[1] = -d1[1];
      position[2] = -d1[2];
      viewUp[0] = d2[0];
      viewUp[1] = d2[1];
      viewUp[2] = d2[2];
      break;
    default:  // kPlaneSagittal
      position[0] = d0[0];
      position[1] = d0[1];
      position[2] = d0[2];
      viewUp[0] = d2[0];
      viewUp[1] = d2[1];
      viewUp[2] = d2[2];
      break;
  }
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetPosition(position);
  camera->SetViewUp(viewUp);
}

void VtkMprRenderer::reset()
{
  if (!m_volume) {
    return;
  }
  for (int plane = 0; plane < kPlaneCount; ++plane) {
    if (m_windows[plane]) {
      // 重申朝向 + fit（style 为 IMAGE2D 模式，用户不能旋转相机，
      // 重申无副作用）——兜底保证窗口切换/上下文重建后图像可见
      updateCameraOrientation(plane);
      m_renderers[plane]->ResetCamera();
    }
  }
  render();
}

void VtkMprRenderer::renderPlane(int plane)
{
  // 只绘已绑定且有体的子视口：无体早退复刻 vtkImageViewer2::
  // Render 的 `if (GetInput()) RenderWindow->Render()`——占位态
  // （面板切到占位、widget 隐藏）不触碰 GL
  if (m_windows[plane] && m_volume) {
    m_windows[plane]->Render();
  }
}

void VtkMprRenderer::render()
{
  for (int plane = 0; plane < kPlaneCount; ++plane) {
    renderPlane(plane);
  }
}

void VtkMprRenderer::setSlicePosition(int plane, double position)
{
  if (!m_volume || plane < 0 || plane >= kPlaneCount || !m_windows[plane]) {
    // 无体 no-op；渲染目标未注入（activate 前）时跳过——activate 后
    // 由面板补推切面（KTD8），避免在默认窗上消费 FirstRender 状态。
    return;
  }
  int extent[6];
  m_volume->GetExtent(extent);
  const int axis = kPlaneAxis[plane];
  // 夹取到该平面数据轴的 extent（等价旧 vtkImageViewer2::SetSlice 的
  // SliceRange 夹取）
  const int index = std::clamp(static_cast<int>(std::lround(position)),
                               extent[2 * axis],
                               extent[2 * axis + 1]);
  if (index == m_sliceIndex[plane]) {
    // 同值幂等：不写不绘（复刻 SetSlice 早退；activate 的重复补推
    // 零成本）。切面状态只在本对象内变更，不经交互路径，因此不会
    // 触发回灌回调——程序化写入天然静默（防环）。
    return;
  }
  m_sliceIndex[plane] = index;
  applySlice(plane);
  renderPlane(plane);
}

void VtkMprRenderer::setWindowLevelCallback(
    std::function<void(double, double)> callback)
{
  // 观察者在构造时已挂接；此处只登记回灌目标（null = 不回灌）
  m_windowLevelCallback = std::move(callback);
}

void VtkMprRenderer::setSliceIndexCallback(std::function<void(double)> callback)
{
  m_sliceIndexCallback = std::move(callback);
}

int VtkMprRenderer::planeOfStyle(const vtkObject* caller) const
{
  for (int plane = 0; plane < kPlaneCount; ++plane) {
    if (m_styles[plane].Get() == caller) {
      return plane;
    }
  }
  return -1;
}

void VtkMprRenderer::handleStartWindowLevel(vtkObject* caller,
                                            unsigned long,
                                            void*)
{
  const int plane = planeOfStyle(caller);
  if (plane < 0 || !m_volume) {
    return;
  }
  // KTD11 计算基准：拖拽开始时刻该子视口的当前窗宽窗位（property
  // 是显示真值；三平面同值，任意 pane 的值即全局值）
  auto* property = m_slices[plane]->GetProperty();
  m_initialWindowWidth = property->GetColorWindow();
  m_initialWindowCenter = property->GetColorLevel();
}

void VtkMprRenderer::handleWindowLevel(vtkObject* caller, unsigned long, void*)
{
  if (m_suppressWindowLevelCallback) {
    // 程序化回写期间的回灌直接丢弃（KTD11 第一道闸）
    return;
  }
  const int plane = planeOfStyle(caller);
  if (plane < 0 || !m_windows[plane] || !m_volume) {
    return;
  }
  const int* size = m_windows[plane]->GetSize();
  if (size[0] <= 0 || size[1] <= 0) {
    return;
  }

  // 计算公式在 WindowLevelDrag 中单源（复刻 vtkInteractorStyleImage::
  // WindowLevel，style 在有观察者时把应用职责交给观察者，见其源码）
  const int* start = m_styles[plane]->GetWindowLevelStartPosition();
  const int* current = m_styles[plane]->GetWindowLevelCurrentPosition();
  double newWindow = 0.0;
  double newLevel = 0.0;
  WindowLevelDrag::compute(start,
                           current,
                           size,
                           m_initialWindowWidth,
                           m_initialWindowCenter,
                           newWindow,
                           newLevel);

  // 应用到三联 + 主动重绘
  updateWindowLevel(newWindow, newLevel);
  if (m_windowLevelCallback) {
    m_windowLevelCallback(newWindow, newLevel);
  }
}

void VtkMprRenderer::handleSliceWheel(int plane, int sign)
{
  if (!m_volume || plane < 0 || plane >= kPlaneCount || !m_windows[plane]
      || sign == 0)
  {
    // 无体/未绑定安全空操作（与旧回调的守卫一致）
    return;
  }
  int extent[6];
  m_volume->GetExtent(extent);
  const int axis = kPlaneAxis[plane];
  const int index = std::clamp(
      m_sliceIndex[plane] + sign, extent[2 * axis], extent[2 * axis + 1]);
  if (index == m_sliceIndex[plane]) {
    // 边界不动不绘、不回灌（旧 IncrementSlice 切层失败不发事件同语义）
    return;
  }
  m_sliceIndex[plane] = index;
  applySlice(plane);
  renderPlane(plane);

  // 仅轴位 pane（数据轴 = z）携带体素索引回灌 → 2D 跳层；冠状/矢状
  // 的切面是 y/x，不影响 2D 层索引（R8 反向链路的"只有 z 变化才跳层"，
  // M3 仅承诺轴位采集序列的直映射）。回灌链：setCurrentIndex 同值
  // 幂等 → 最多一次 sliceChanged → 轴位面板 setSlicePosition 回推同值
  // → 早退，无二次绘、无环。
  if (plane == IMprRenderer::kPlaneAxial && m_sliceIndexCallback) {
    m_sliceIndexCallback(static_cast<double>(index));
  }
}
