# deprecated —— 暂离构建的夹具依赖测试

## 为什么在这里

`res/series/` 多层轴位序列夹具（`slice_*.dcm` + `expected.txt`）已从
仓库删除，所有依赖它的测试用例会以

```
缺少夹具期望文件: res/series/expected.txt
```

或夹具目录打开失败而挂掉。按约定：**先忽略这批测试，等需要时再处理**，
故整体挪到本目录（不参与 CMake 构建，`test/CMakeLists.txt` 不引用）。

生成脚本 `tools/gen_series_fixture.py` 保留在原处，夹具可随时再生：

```bash
pip install pydicom
python3 tools/gen_series_fixture.py --n 12
```

## 文件对照

| 本目录文件 | 活动版剩余内容 |
|------|------|
| `SeriesRead_test.cpp` | `InvalidPathsThrowCatchable`（不依赖夹具） |
| `LoadSeriesUseCase_test.cpp` | `MissingWindowLevelUsesDefaults`、`InvalidPathFailsWithResultError` |
| `SeriesViewModel_test.cpp` | `NavigationBeforeLoadIsSafeNoop`、`SingleFrameSeriesNavigationIsNoop` |
| `MprPanel_test.cpp` | 注册 / 槽位 / 占位 / no-op / activate 补调等 9 个用例 |
| `TestCommon.h` | 仅 `waitFor`（期望值解析 `SeriesExpectation` / `loadExpectation` / `fixtureFrameCount` 在此） |

## 恢复步骤

1. `pip install pydicom && python3 tools/gen_series_fixture.py`
2. 把本目录的夹具依赖用例合回对应的活动测试文件（或直接以本目录版本
   覆盖，再补回活动版保留的用例）。
3. `TestCommon.h` 补回期望值解析部分（以本目录版本为准）。
4. 重新生成夹具后运行 `ctest --test-dir build` 核对。
