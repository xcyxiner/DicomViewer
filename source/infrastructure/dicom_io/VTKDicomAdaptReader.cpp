
#include <algorithm>
#include <filesystem>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "VTKDicomAdaptReader.h"

#include <vtkDICOMDirectory.h>
#include <vtkDICOMMetaData.h>
#include <vtkDICOMTagPath.h>
#include <vtkDoubleArray.h>
#include <vtkErrorCode.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>

#include "infrastructure/utils/ImageSliceExtractor.h"

VTKDicomAdaptReader::VTKDicomAdaptReader()
{
  m_reader = vtkSmartPointer<vtkDICOMReader>::New();
}

void VTKDicomAdaptReader::open(const std::string& filePath)
{
  close();
  m_filePath = filePath;

  // 单次 status 查询完成路径分类（目录/普通文件/其余），消除
  // is_directory + is_regular_file 的重复 stat；错误消息与分支
  // 行为同原先逐次检查完全一致
  std::error_code ec;
  const auto fileStatus = std::filesystem::status(filePath, ec);
  if (!ec && std::filesystem::is_directory(fileStatus)) {
    // 目录：vtkDICOMDirectory 枚举 → 取文件数最多的序列 → SetFileNames
    // （vtk-dicom 0.8.17 的 reader 无 SetDirectoryName，排序由 reader
    //  按 IPP/IOP 自动完成 —— KTD5）
    auto directory = vtkSmartPointer<vtkDICOMDirectory>::New();
    directory->SetDirectoryName(filePath.c_str());
    directory->Update();

    const int numSeries = directory->GetNumberOfSeries();
    if (numSeries <= 0) {
      throw std::runtime_error("目录中未找到 DICOM 序列: " + filePath);
    }
    const auto countOf = [&directory](int series)
    {
      auto* names = directory->GetFileNamesForSeries(series);
      return names ? static_cast<int>(names->GetNumberOfValues()) : 0;
    };
    // std::max_element 等大时保留首元素，与原循环的严格大于更新
    // 语义一致（保首个最大序列）
    std::vector<int> seriesIndices(static_cast<size_t>(numSeries));
    std::iota(seriesIndices.begin(), seriesIndices.end(), 0);
    const int bestSeries = *std::max_element(
        seriesIndices.begin(),
        seriesIndices.end(),
        [&countOf](int a, int b) { return countOf(a) < countOf(b); });
    m_fileNames = vtkSmartPointer<vtkStringArray>::New();
    m_fileNames->DeepCopy(directory->GetFileNamesForSeries(bestSeries));
    if (m_fileNames->GetNumberOfValues() == 0) {
      throw std::runtime_error("目录中未找到 DICOM 序列: " + filePath);
    }
  } else {
    if (ec || !std::filesystem::is_regular_file(fileStatus)) {
      throw std::runtime_error("路径不存在或不可读: " + filePath);
    }
    m_fileNames = vtkSmartPointer<vtkStringArray>::New();
    m_fileNames->InsertNextValue(filePath);
  }

  m_reader->SetFileNames(m_fileNames);
  m_reader->SetMemoryRowOrderToFileNative();
  m_reader->Update();

  // 校验读取结果：无效输入抛出可捕获异常（R9，禁止越过线程边界）
  auto* output = m_reader->GetOutput();
  auto* meta = m_reader->GetMetaData();
  int extent[6] = {0, -1, 0, -1, 0, -1};
  if (output) {
    output->GetExtent(extent);
  }
  if (m_reader->GetErrorCode() != vtkErrorCode::NoError || !output || !meta
      || meta->GetNumberOfInstances() == 0 || extent[1] < extent[0]
      || extent[3] < extent[2] || extent[5] < extent[4])
  {
    close();
    throw std::runtime_error("无法作为 DICOM 序列打开: " + filePath);
  }

  // KTD6：DeepCopy 患者矩阵（reader 持有的矩阵在下次 open 时会被重算）
  // 并把旋转/平移部分烘入体的方向矩阵与原点，此后体即患者空间。
  m_patientMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  m_patientMatrix->DeepCopy(m_reader->GetPatientMatrix());
  bakePatientGeometry();

  // 采样体快照：几何为独立副本、像素数组只增引用计数（见 m_volume）。
  // 快照浅共享使 reader 下次 Update 因引用计数 >1 而分配新缓冲，
  // 已采纳的旧体在下一次加载期间保持稳定（KTD9）。
  m_volume = vtkSmartPointer<vtkImageData>::New();
  m_volume->ShallowCopy(output);
}

Series VTKDicomAdaptReader::readSeries(const std::string& path)
{
  return Series();
}

bool VTKDicomAdaptReader::mapSliceIndex(int index, int& fileIndex) const
{
  if (!m_reader) {
    return false;
  }
  auto* fileArray = m_reader->GetFileIndexArray();
  auto* frameArray = m_reader->GetFrameIndexArray();
  // frameIndex 出参已无消费方（像素取自体的 z extent），但其非空仍作
  // 有效性检查保留——缺失 FrameIndexArray 的 reader 状态视为未就绪
  if (!fileArray || !frameArray || index < 0
      || index >= fileArray->GetNumberOfTuples())
  {
    return false;
  }
  // 切片索引 ≠ 文件索引，必须经索引数组映射（KTD5）
  fileIndex = fileArray->GetComponent(index, 0);
  return true;
}

namespace
{

// readFrameInfo 与 readFrame 共用的三个属性查询（按文件实例索引取
// 该层的 SOP Instance UID 与 WW/WC 标签值）。只提供取值：有效性门控
// 由两个调用方按各自语义处理（readFrame 的 FieldData 挂载要求
// uid 非空、wc&&ww 同时有效，与 Frame 的逐项判效不同，不得经
// Frame 中转——其默认值 0.0 会使门控不可分辨）。
struct LayerTags
{
  vtkDICOMValue sopInstanceUid;
  vtkDICOMValue windowWidth;
  vtkDICOMValue windowCenter;
};

LayerTags fetchLayerTags(vtkDICOMMetaData* meta, int fileIndex)
{
  LayerTags tags;
  tags.sopInstanceUid = meta->GetAttributeValue(fileIndex, DC::SOPInstanceUID);
  tags.windowWidth = meta->GetAttributeValue(fileIndex, DC::WindowWidth);
  tags.windowCenter = meta->GetAttributeValue(fileIndex, DC::WindowCenter);
  return tags;
}

}  // namespace

std::unique_ptr<Frame> VTKDicomAdaptReader::readFrameInfo(int index)
{
  int fileIndex = -1;
  if (!mapSliceIndex(index, fileIndex)) {
    return {};
  }
  auto* meta = m_reader->GetMetaData();
  if (!meta) {
    return {};
  }

  auto frame = std::make_unique<Frame>();
  frame->setFrameIndex(index);

  const LayerTags tags = fetchLayerTags(meta, fileIndex);
  if (tags.sopInstanceUid.IsValid()) {
    frame->setSopInstanceUid(tags.sopInstanceUid.AsString());
  }

  // WW/WC 多值时取第一个（与 readFrame 的 FieldData 约定一致）
  if (tags.windowWidth.IsValid()) {
    frame->setWindowWidth(tags.windowWidth.AsDouble());
  }
  if (tags.windowCenter.IsValid()) {
    frame->setWindowCenter(tags.windowCenter.AsDouble());
  }

  auto rowsValue = meta->GetAttributeValue(fileIndex, DC::Rows);
  if (rowsValue.IsValid()) {
    frame->setRows(rowsValue.AsInt());
  }
  auto colsValue = meta->GetAttributeValue(fileIndex, DC::Columns);
  if (colsValue.IsValid()) {
    frame->setCols(colsValue.AsInt());
  }
  // 文件读入的 DS 值以字符串形式存储，须经 GetValues 做数值转换
  auto spacingValue = meta->GetAttributeValue(fileIndex, DC::PixelSpacing);
  if (spacingValue.IsValid() && spacingValue.GetNumberOfValues() >= 2) {
    double spacing[2] = {0.0, 0.0};
    spacingValue.GetValues(spacing, 2, 0);
    frame->setPixelSpacingX(spacing[0]);
    frame->setPixelSpacingY(spacing[1]);
  }

  auto ippValue = meta->GetAttributeValue(fileIndex, DC::ImagePositionPatient);
  if (ippValue.IsValid() && ippValue.GetNumberOfValues() >= 3) {
    double ipp[3] = {0.0, 0.0, 0.0};
    ippValue.GetValues(ipp, 3, 0);
    std::array<double, 3> position = {ipp[0], ipp[1], ipp[2]};
    frame->setImagePositionPatient(position);
  }
  auto iopValue =
      meta->GetAttributeValue(fileIndex, DC::ImageOrientationPatient);
  if (iopValue.IsValid() && iopValue.GetNumberOfValues() >= 6) {
    double iop[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    iopValue.GetValues(iop, 6, 0);
    std::array<double, 6> orientation;
    for (int i = 0; i < 6; ++i) {
      orientation[i] = iop[i];
    }
    frame->setImageOrientationPatient(orientation);
  }

  // 体数据已由 reader AutoRescale 输出 HU，Frame 侧记录原始标签值
  auto slopeValue = meta->GetAttributeValue(fileIndex, DC::RescaleSlope);
  frame->setSlope(slopeValue.IsValid() ? slopeValue.AsDouble() : 1.0);
  auto interceptValue =
      meta->GetAttributeValue(fileIndex, DC::RescaleIntercept);
  frame->setIntercept(interceptValue.IsValid() ? interceptValue.AsDouble()
                                               : 0.0);
  return frame;
}

IFrameCache::FramePtr VTKDicomAdaptReader::readFrame(int index)
{
  int fileIndex = -1;
  if (!mapSliceIndex(index, fileIndex)) {
    return nullptr;
  }
  auto* meta = m_reader->GetMetaData();
  if (!m_volume || !meta) {
    return nullptr;
  }

  // 像素抽取委托唯一抽层实现：从已采纳的体快照取 extent z=index 的
  // 单层，origin/direction/spacing 原样携带，抽层与体（及 MPR）消费
  // 同一份患者几何（KTD6）；快照在加载期间稳定，抽层不与后台 open
  // 竞争（KTD9）。GUI 侧（SeriesViewModel::render）经同一工具函数
  // 对体快照抽层，不经过本 reader。
  auto slice = ImageSliceExtractor::extractSlice(m_volume, index);
  if (!slice) {
    return nullptr;
  }

  // 逐层元数据挂 FieldData：实例索引取该层，禁止硬编码 0（KTD5）。
  // 取值经共享 fetchLayerTags，门控条件保持原样（uid 非空 / wc&&ww
  // 同时有效）。
  const LayerTags tags = fetchLayerTags(meta, fileIndex);
  if (tags.sopInstanceUid.IsValid() && !tags.sopInstanceUid.AsString().empty())
  {
    auto uidArray = vtkSmartPointer<vtkStringArray>::New();
    uidArray->SetName("SOPInstanceUIDs");
    uidArray->InsertNextValue(tags.sopInstanceUid.AsString());
    slice->GetFieldData()->AddArray(uidArray);
  }

  if (tags.windowCenter.IsValid() && tags.windowWidth.IsValid()) {
    auto wcArray = vtkSmartPointer<vtkDoubleArray>::New();
    wcArray->SetName("WindowCenter");
    wcArray->InsertNextValue(tags.windowCenter.AsDouble());
    slice->GetFieldData()->AddArray(wcArray);

    auto wwArray = vtkSmartPointer<vtkDoubleArray>::New();
    wwArray->SetName("WindowWidth");
    wwArray->InsertNextValue(tags.windowWidth.AsDouble());
    slice->GetFieldData()->AddArray(wwArray);
  }

  return slice;
}

vtkSmartPointer<vtkImageData> VTKDicomAdaptReader::readVolume()
{
  // open() 时采样的快照：几何已烘入，像素数组与 reader 输出浅共享
  // （不复制像素）。失败的 open 不触碰快照，旧体保持可用（R9）。
  return m_volume;
}

int VTKDicomAdaptReader::getFrameCount()
{
  if (!m_volume) {
    return 0;
  }
  int extent[6];
  m_volume->GetExtent(extent);
  if (extent[5] < extent[4]) {
    return 0;
  }
  return extent[5] - extent[4] + 1;
}

std::string VTKDicomAdaptReader::getSeriesInstanceUid()
{
  if (!m_reader) {
    return {};
  }
  auto* meta = m_reader->GetMetaData();
  if (!meta || meta->GetNumberOfInstances() == 0) {
    return {};
  }
  auto value = meta->GetAttributeValue(0, DC::SeriesInstanceUID);
  return value.IsValid() ? value.AsString() : std::string {};
}

void VTKDicomAdaptReader::close()
{
  if (m_reader) {
    m_reader->SetFileName(nullptr);
    m_reader->SetFileNames(nullptr);
  }
  m_fileNames = nullptr;
  m_patientMatrix = nullptr;
  m_filePath.clear();
  // m_volume 有意保留：失败的 open（open() 先 close()）不应使已采纳的
  // 旧体失效——已显示图像与抽层路径须保持可用（R9）；下一次成功的
  // open 会整体替换它。
}

void VTKDicomAdaptReader::bakePatientGeometry()
{
  auto* output = m_reader->GetOutput();
  if (!output || !m_patientMatrix) {
    return;
  }
  // 平移部分 → 原点；旋转部分 → 方向矩阵（spacing 由 reader 独立给出）
  output->SetOrigin(m_patientMatrix->GetElement(0, 3),
                    m_patientMatrix->GetElement(1, 3),
                    m_patientMatrix->GetElement(2, 3));
  auto direction = vtkSmartPointer<vtkMatrix3x3>::New();
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      direction->SetElement(row, col, m_patientMatrix->GetElement(row, col));
    }
  }
  output->SetDirectionMatrix(direction);
  output->Modified();
}
