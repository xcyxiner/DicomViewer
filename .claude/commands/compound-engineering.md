# Compound Engineering — DicomViewer (MVVM + DDD)

执行完整的工程化开发流程，适用于 DicomViewer 项目的 MVVM + DDD 架构。

---

## Phase 1: Plan（规划）

**目标**: 理解需求，制定实施方案，确认架构分层。

1. **需求分析**
   - 明确功能目标和技术约束
   - 确认影响的架构层级（Domain / Application / Infrastructure / Presentation）

2. **架构影响评估**
   - 判断是否涉及领域模型变更（Patient/Study/Series/Image/Frame）
   - 判断是否需要新增基础设施接口（I 前缀抽象接口）
   - 判断是否需要新增用例（UseCase）
   - 判断是否涉及 View/ViewModel 变更

3. **方案设计**
   - 确定涉及的文件和类
   - 确定接口定义和依赖关系
   - 确定测试策略

**输出**: 一份结构化的实施计划，包含：
- 需要修改/新增的文件列表（按架构层级分组）
- 接口签名（如有新增）
- 依赖注入方案
- 测试计划

---

## Phase 2: Domain（领域层实现）

**规则**:
- 纯 C++ 实体，零框架依赖
- 使用 `m_` 前缀命名私有成员
- 聚合根通过 `unique_ptr` 拥有子实体
- 子实体通过裸指针 `m_parent` 指向父实体
- 值对象使用 struct，聚合使用 class

**检查清单**:
- [ ] 无 `#include <QObject>` 或任何 Qt/VTK/DCMTK 头文件
- [ ] 使用 `std::unique_ptr` 管理子集合
- [ ] 提供必要的 getter/setter
- [ ] 正确实现移动语义

---

## Phase 3: Infrastructure（基础设施层实现）

**规则**:
- 接口以 `I` 前缀命名（如 `IDicomReader`）
- 实现类与接口同目录
- 使用构造函数注入依赖
- `FramePtr` variant 类型用于多态帧数据

**检查清单**:
- [ ] 接口定义为纯虚类
- [ ] 实现类遵循接口契约
- [ ] 使用 `std::future` 处理异步
- [ ] 在 `source_list.cmake` 中注册新文件（或运行 `gen_src.py`）

---

## Phase 4: Application（应用层实现）

**规则**:
- 用例类编排领域对象和基础设施接口
- 通过构造函数注入 `IDicomReader&`, `ITaskQueue&`, `IFrameCache&` 等
- 不直接依赖具体实现类

**检查清单**:
- [ ] 用例类不包含 UI 逻辑
- [ ] 异步操作返回 `std::future`
- [ ] 错误通过异常或 `std::expected` 传播

---

## Phase 5: Presentation（表现层实现）

### ViewModel
- 继承 `QObject`，持有基础设施具体实例
- 暴露公共方法给 View 调用
- 通过信号（`emit`）通知 View 更新

### View
- 使用 `.ui` 文件定义布局
- 通过信号/槽绑定 ViewModel
- 不包含业务逻辑

**检查清单**:
- [ ] ViewModel 不直接操作 UI 控件
- [ ] View 不直接调用基础设施接口
- [ ] 信号/槽连接在 View 构造函数中完成
- [ ] `.ui` 文件正确注册

---

## Phase 6: Test（测试）

- 测试框架: Google Test
- 测试文件: `test/` 目录
- 测试数据: `res/*.dcm`

**检查清单**:
- [ ] 新增接口的单元测试
- [ ] 覆盖正常路径和异常路径
- [ ] 运行 `ctest --test-dir build` 通过

---

## Phase 7: Verify（验证）

1. **编译检查**
   ```bash
   cmake --build build
   ```

2. **测试通过**
   ```bash
   ctest --test-dir build
   ```

3. **代码规范**
   ```bash
   cmake --build build --target format
   cmake --build build --target spell
   ```

4. **架构合规**
   - 领域层无框架依赖
   - 依赖方向正确: Presentation → Application → Infrastructure → Domain
   - 接口隔离: 基础设施通过抽象接口解耦

---

## 快速参考

### 关键文件位置
| 层级 | 路径 |
|------|------|
| 领域模型 | `source/domain/model/` |
| 用例 | `source/application/` |
| DICOM 接口/实现 | `source/infrastructure/dicom_io/` |
| 渲染接口/实现 | `source/infrastructure/rendering/` |
| 缓存接口/实现 | `source/infrastructure/cache/` |
| 任务接口/实现 | `source/infrastructure/task/` |
| ViewModel | `source/presentation/viewmodels/` |
| View | `source/presentation/view/` |

### 脚手架工具
```bash
python3 tools/new_c_class.py      # 新增 C++ 类
python3 tools/new_qt_object.py    # 新增 Qt 对象
python3 gen_src.py                 # 更新 source_list.cmake
```
