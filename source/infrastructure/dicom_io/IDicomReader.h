
#pragma once
#include <string>
#include <memory>
#include "domain/model/Series.h"
#include "domain/model/Frame.h"
#include "infrastructure/cache/IFrameCache.h"
class IDicomReader
{
public:
    explicit IDicomReader();
    virtual ~IDicomReader()=default;

    // 打开一个 DICOM 文件或序列目录。
    // 处理流程：文件路径直接 SetFileName；目录路径经 vtkDICOMDirectory 枚举
    // 目录内文件，选定目标序列后以 SetFileNames 喂给 reader，
    // 由 reader 按 ImagePositionPatient/ImageOrientationPatient 自动排序堆叠，
    // 一次 Update 得到整幅 3D 体。
    virtual void open(const std::string& filePath) = 0;

    // 返回序列对应的领域 Series（UID 等元信息）。
    // 处理流程：从 vtkDICOMDirectory 的 SeriesRecord 填充领域对象，
    // 不引入框架类型（当前为占位，消费方接入后补实）。
    virtual Series readSeries(const std::string& path) = 0;

    // 返回第 index 层的帧元数据（SOP Instance UID、WW/WC 等）。
    // 处理流程：index 为序列内层号，须经 GetFileIndexArray/GetFrameIndexArray
    // 映射到文件实例后读取该层元数据（切片索引 ≠ 文件索引，禁止硬编码 0）。
    virtual std::unique_ptr<Frame> readFrameInfo( int index) = 0;

    // 读取第 index 层的像素数据。
    // 处理流程：与 readFrameInfo 相同的索引映射取该层元数据，
    // 再从整幅体上抽取该层（全计划唯一的抽层实现，表现层复用此处）；
    // 输出供测试与元数据使用；体为唯一像素源后逐帧像素不入缓存
    // （缓存填充职责已随 KTD1 移除）。
    virtual IFrameCache::FramePtr readFrame(int index) = 0;

    // 序列总层数，open() 成功后有效。
    // 处理流程：由体数据 extent 或元数据实例数得出。
    // 基类默认返回 0，由 VTK 实现覆写（范围外实现无需改动即可编译）。
    virtual int getFrameCount() { return 0; }

    // 序列缓存键（SeriesInstanceUID），open() 成功后有效。
    // 处理流程：从元数据读取，作为 IFrameCache 体槽位的 key。
    // 基类默认返回空串，由 VTK 实现覆写。
    virtual std::string getSeriesInstanceUid() { return {}; }

    // 整幅 3D 体数据 —— 序列的唯一像素源。
    // 处理流程：open() 时 reader 已一次 Update 载入；采纳（进缓存）前
    // 须 DeepCopy GetPatientMatrix 并把患者几何（方向 + 原点）烘入体，
    // 此后体即患者空间，2D 与 MPR 消费同一几何。
    // 基类默认返回空指针，由 VTK 实现覆写。
    virtual vtkSmartPointer<vtkImageData> readVolume() { return nullptr; }

    // 关闭当前序列，释放 reader 侧资源（打开新序列前调用）。
    virtual void close() = 0;
};
