<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/github/license/xcyxiner/DicomViewer" alt="License"></a>
  <a href="https://github.com/xcyxiner/DicomViewer/actions"><img src="https://img.shields.io/github/actions/workflow/status/xcyxiner/DicomViewer/ci.yml?style=flat-square" alt="CI"></a>
  <a href="https://github.com/xcyxiner/DicomViewer/commits/main"><img src="https://img.shields.io/github/last-commit/xcyxiner/DicomViewer?style=flat-square" alt="Last Commit"></a>
  <a href="https://github.com/xcyxiner/DicomViewer/stargazers"><img src="https://img.shields.io/github/stars/xcyxiner/DicomViewer?style=social" alt="Stars"></a>
</p>

# DicomViewer

This is the DicomViewer project.

## 目标
2D阅片
MPR
3D

## 参考资料
### 测试数据
[dicom-test-files.git](https://github.com/robyoung/dicom-test-files.git)

## 依赖

```
系统 ubuntu 24.04(wsl)
qt6.4
依赖dcmtk gdcm vtk vtk-dicom(gdcm)
```

## IDE以及开发环境
```
win11
wsl2
nvim
vscode
```

## 文档记录
- [qt以及opengl 专栏](https://juejin.cn/column/7645237921838972978)
- [DicomViewer 1](https://juejin.cn/post/7652620835596976171)
- [DicomViewer (目录调整) 2](https://juejin.cn/post/7652286706327699498)
- [DicomViewer (添加模型类)3](https://juejin.cn/post/7652563235685531667)
- [DicomViewer (后台线程处理文件)4](https://juejin.cn/post/7652719984766320694)
- [DicomViewer (dcmtk读取dcm文件)5](https://juejin.cn/post/7652943828446806058)
- [DicomViewer (dcm模型类赋值)6](https://juejin.cn/post/7653651196972433427)
- [DicomViewer (vcpkg Windows和ubuntu编译)7](https://juejin.cn/post/7655901733764694079)
- [DicomViewer 8(编译以及优化)](https://juejin.cn/post/7658236946784747526)
- [DicomViewer 9(ui编辑以及引用)](https://juejin.cn/post/7658478348117704747)
- [DicomViewer 10(使用xxd查看dcm文件)](https://juejin.cn/post/7660788416040943656)
- [DicomViewer 11(vscode 连接wsl gdb断点)](https://juejin.cn/post/7660773919465373711)
- [DicomViewer12(架构更新)](https://juejin.cn/post/7661160061000269864)
- [DicomViewer13(vtk测试)](https://juejin.cn/post/7663505563164327936)
- [DicomViewer14(读取图像按固定窗宽窗位显示)](https://juejin.cn/post/7666250786229239842)
- [DicomViewer15(act验证ci)](https://juejin.cn/post/7668906037449736198)

## 目录结构
```
── 1.ps1
├── 1.sh
├── 2.sh
├── 3.sh
├── 4.sh
├── 5.sh
├── BUILDING.md
├── CMakeLists.txt
├── CMakePresets.json
├── CMakeUserPresets.json
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── Dockerfile
├── HACKING.md
├── LICENSE
├── README.md
├── cmake
│   ├── coverage.cmake
│   ├── dev-mode.cmake
│   ├── docs-ci.cmake
│   ├── docs.cmake
│   ├── folders.cmake
│   ├── install-rules.cmake
│   ├── lint-targets.cmake
│   ├── lint.cmake
│   ├── prelude.cmake
│   ├── project-is-top-level.cmake
│   ├── spell-targets.cmake
│   ├── spell.cmake
│   └── variables.cmake
├── gen_src.py
├── res
│   ├── CT_small.dcm
│   ├── ExplVR_LitEndNoMeta.dcm
│   ├── JPEG-LL.dcm
│   ├── JPEG2000.dcm
│   └── MR_small.dcm
├── source
│   ├── application
│   │   ├── LoadSeriesUseCase.cpp
│   │   └── LoadSeriesUseCase.h
│   ├── domain
│   │   └── model
│   ├── infrastructure
│   │   ├── cache
│   │   ├── dicom_io
│   │   ├── file_import
│   │   ├── rendering
│   │   └── task
│   ├── main.cpp
│   └── presentation
│       ├── view
│       └── viewmodels
├── source_list.cmake
├── test
│   ├── CMakeLists.txt
│   └── DcmtkReader_test.cpp
├── tools
│   ├── new_c_class.py
│   └── new_qt_object.py
├── triplets
│   ├── x64-linux.cmake
│   ├── x64-windows-release.cmake
│   └── x64-windows.cmake
├── vcpkg-cache
│   └── downloads
│       ├── k202009-libaec-v1.1.6.tar.gz
│       └── libuuid-1.0.3.tar.gz
├── vcpkg.json
```

# Building and installing
See the [BUILDING](BUILDING.md) document.

```
cmake --preset wsl-debug
cmake --build --preset wsl-debug
```

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

<!--
Please go to https://choosealicense.com/licenses/ and choose a license that
fits your needs. The recommended license for a project of this type is the
GNU AGPLv3.
-->
