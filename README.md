# DicomViewer

This is the DicomViewer project.

## 目标
读取并显示DICM格式

## 依赖
系统 ubuntu 24.04
qt6.4
依赖dcmtk vtk vtk-dicom(gdcm)
参考 [dcmtk vtk vtk-dicom(gdcm) 编译](https://juejin.cn/post/7650428235189108779)

## IDE以及开发环境
win11
wsl2
nvim

# Building and installing
See the [BUILDING](BUILDING.md) document.

```
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug
cmake --build build
```

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

<!--
Please go to https://choosealicense.com/licenses/ and choose a license that
fits your needs. The recommended license for a project of this type is the
GNU AGPLv3.
-->
