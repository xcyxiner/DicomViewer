
#pragma once
#include "infrastructure/dicom_io/IDicomReader.h"
#include <vtkDICOMReader.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
class VTKDicomAdaptReader:public IDicomReader
{
public:
    explicit VTKDicomAdaptReader();
    ~VTKDicomAdaptReader()=default;
public:
    // 打开文件或目录：目录经 vtkDICOMDirectory 枚举、取文件数最多的序列，
    // 与单文件统一走 SetFileNames（SetMemoryRowOrderToFileNative），
    // 一次 Update 得到整幅体；随后 DeepCopy PatientMatrix 并把患者几何
    // 烘入体（KTD5/KTD6）。输入无效（不存在 / 空目录 / 非 DICOM / 解析
    // 失败）抛 std::runtime_error，保证异常可在线程边界内捕获。
    void open(const std::string& filePath) override;

    // 占位：领域 Series 树的消费属计划 Deferred 范围，返回空对象。
    Series readSeries(const std::string& path) override;

    // 经 GetFileIndexArray/GetFrameIndexArray 把层号映射为文件/帧实例，
    // 取该层元数据填入领域 Frame（SOP UID、IPP/IOP、WW/WC、行列、
    // 斜率截距）；层号越界返回空指针。
    std::unique_ptr<Frame> readFrameInfo( int index) override;

    // 同一索引映射取该层元数据，再从已采纳的体快照抽取 extent
    // z=index 的单层（全计划唯一抽层实现，保留 origin/direction/spacing
    // 患者几何），逐层元数据挂 FieldData：SOPInstanceUIDs /
    // WindowCenter / WindowWidth（实例索引而非 0）；层号越界或尚未
    // 打开返回 nullptr 变体。
    IFrameCache::FramePtr readFrame(int index) override;

    // 已采纳的整幅体快照（open 成功时以浅共享方式采样：几何独立、
    // 像素数组与 reader 输出共享引用，见 m_volume 注释）。
    vtkSmartPointer<vtkImageData> readVolume() override;

    // 体 z 方向层数（最后一次成功 open 后有效，未打开返回 0）。
    int getFrameCount() override;

    // 元数据 DC::SeriesInstanceUID（open 成功后有效，未打开返回空串）。
    std::string getSeriesInstanceUid() override;

    // 清空 reader 输入与本类状态（打开新序列前由 open() 调用）。
    void close() override;

private:
    // 层号 → 文件实例索引；reader 未打开、FrameIndexArray 缺失或
    // 层号越界返回 false（frameIndex 出参已无消费方，一并移除）。
    bool mapSliceIndex(int index, int& fileIndex) const;

    // 把 m_patientMatrix 的旋转部分烘入体的方向矩阵、平移部分烘入原点
    // （KTD6），此后体即患者空间。
    void bakePatientGeometry();

    std::string m_filePath;
    vtkSmartPointer<vtkDICOMReader> m_reader;
    // open() 时 DeepCopy 的患者矩阵：reader 持有的矩阵会在下次 open 时
    // 被重算，不可跨序列持有其裸指针。
    vtkSmartPointer<vtkMatrix4x4> m_patientMatrix;
    // reader 以注册方式持有文件名列表（SetFileNames），本成员保证列表
    // 生命周期覆盖 reader 使用期。
    vtkSmartPointer<vtkStringArray> m_fileNames;
    // 最后一次成功 open 时采样的体快照（readVolume 直接返回）。
    // 浅共享：几何（origin/direction）为独立副本，像素数组只增引用计数
    // —— 计数 >1 迫使 reader 下次 Update 分配新缓冲，已采纳的旧体
    // 不会被后续 open 原地改写（KTD9 "加载期间 GUI 继续用旧体"），
    // 且无逐层/整幅像素副本（KTD1）。
    vtkSmartPointer<vtkImageData> m_volume;
};
